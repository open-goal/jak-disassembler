/*!
 * @file ObjectFileDB.cpp
 * A "database" of object files found in DGO files.
 * Eliminates duplicate object files, and also assigns unique names to all object files
 * (there may be different object files with the same name sometimes)
 */

#include "ObjectFileDB.h"
#include <algorithm>
#include <cstring>
#include "LinkedObjectFileCreation.h"
#include "game_version.h"
#include "minilzo/minilzo.h"
#include "util/BinaryReader.h"
#include "util/FileIO.h"
#include "util/Timer.h"

/*!
 * Get a unique name for this object file.
 */
std::string ObjectFileRecord::to_unique_name() {
  return name + "-v" + std::to_string(version);
}

/*!
 * Build an object file DB for the given list of DGOs.
 */
ObjectFileDB::ObjectFileDB(const std::vector<std::string>& _dgos) {
  Timer timer;

  printf("- Initializing ObjectFileDB...\n");
  for(auto& dgo : _dgos) {
    get_objs_from_dgo(dgo);
  }


  printf("ObjectFileDB Initialized:\n");
  printf(" total dgos: %ld\n", _dgos.size());
  printf(" total data: %d bytes\n", stats.total_dgo_bytes);
  printf(" total objs: %d\n", stats.total_obj_files);
  printf(" unique objs: %d\n", stats.unique_obj_files);
  printf(" unique data: %d bytes\n", stats.unique_obj_bytes);
  printf(" total %.1f ms (%.3f MB/sec, %.3f obj/sec)\n",
         timer.getMs(), stats.total_dgo_bytes / ((1u << 20u) * timer.getSeconds()),
         stats.total_obj_files / timer.getSeconds());
  printf("\n");
}

// Header for a DGO file
struct DgoHeader {
  uint32_t size;
  char name[60];
};

/*!
 * Assert false if the char[] has non-null data after the null terminated string.
 * Used to sanity check the sizes of strings in DGO/object file headers.
 */
static void assert_string_empty_after(const char* str, int size) {
  auto ptr = str;
  while(*ptr) ptr++;
  while(ptr - str < size) {
    assert(!*ptr);
    ptr++;
  }
}

constexpr int MAX_CHUNK_SIZE = 0x8000;
/*!
 * Load the objects stored in the given DGO into the ObjectFileDB
 */
void ObjectFileDB::get_objs_from_dgo(const std::string& filename) {
  auto dgo_data = read_binary_file(filename);
  stats.total_dgo_bytes += dgo_data.size();

  const char jak2_header[] = "oZlB";
  bool is_jak2 = true;
  for(int i = 0; i < 4; i++) {
    if(jak2_header[i] != dgo_data[i]) {
      is_jak2 = false;
    }
  }

  if(is_jak2) {
    if(lzo_init() != LZO_E_OK) {
      assert(false);
    }
    BinaryReader compressed_reader(dgo_data);
    // seek past oZlB
    compressed_reader.ffwd(4);
    auto decompressed_size = compressed_reader.read<uint32_t>();
    std::vector<uint8_t> decompressed_data;
    decompressed_data.resize(decompressed_size);
    size_t output_offset = 0;
    while(true) {
      // seek past alignment bytes and read the next chunk size
      uint32_t chunk_size = 0;
      while(!chunk_size) {
        chunk_size = compressed_reader.read<uint32_t>();
      }

      if(chunk_size < MAX_CHUNK_SIZE) {
        lzo_uint bytes_written;
        auto lzo_rv = lzo1x_decompress(compressed_reader.here(), chunk_size, decompressed_data.data() + output_offset, &bytes_written, nullptr);
        assert(lzo_rv == LZO_E_OK);
        compressed_reader.ffwd(chunk_size);
        output_offset += bytes_written;
      } else {
        // nope - sometimes chunk_size is bigger than MAX, but we should still use max.
//        assert(chunk_size == MAX_CHUNK_SIZE);
        memcpy(decompressed_data.data() + output_offset, compressed_reader.here(), MAX_CHUNK_SIZE);
        compressed_reader.ffwd(MAX_CHUNK_SIZE);
        output_offset += MAX_CHUNK_SIZE;
      }

      if(output_offset >= decompressed_size) break;
      while(compressed_reader.get_seek() %4) {
        compressed_reader.ffwd(1);
      }
    }
    dgo_data = decompressed_data;
  }

  BinaryReader reader(dgo_data);
  auto header = reader.read<DgoHeader>();

  auto dgo_base_name = base_name(filename);
  assert(header.name == dgo_base_name);
  assert_string_empty_after(header.name, 60);


  // get all obj files...
  for(uint32_t i = 0; i < header.size; i++) {
    auto obj_header = reader.read<DgoHeader>();
    assert(reader.bytes_left() >= obj_header.size);
    assert_string_empty_after(obj_header.name, 60);

    add_obj_from_dgo(obj_header.name, reader.here(), obj_header.size, dgo_base_name);
    reader.ffwd(obj_header.size);
  }


  // check we're at the end
  assert(0 == reader.bytes_left());
}

/*!
 * Add an object file to the ObjectFileDB
 */
void ObjectFileDB::add_obj_from_dgo(const std::string &obj_name, uint8_t *obj_data, uint32_t obj_size,
                                    const std::string &dgo_name) {
  stats.total_obj_files++;

  auto hash = crc32(obj_data, obj_size);

  // first, check to see if we already got it...
  for(auto& e : obj_files_by_name[obj_name]) {
    if(e.data.size() == obj_size && e.record.hash == hash) {
      // already got it!
      e.reference_count++;
      auto rec = e.record;
      obj_files_by_dgo[dgo_name].push_back(rec);
      return;
    }
  }

  // nope, have to add a new one.
  ObjectFileData data;
  data.data.resize(obj_size);
  memcpy(data.data.data(), obj_data, obj_size);
  data.record.hash = hash;
  data.record.name = obj_name;
  data.record.version = obj_files_by_name[obj_name].size();
  obj_files_by_dgo[dgo_name].push_back(data.record);
  obj_files_by_name[obj_name].emplace_back(std::move(data));
  stats.unique_obj_files++;
  stats.unique_obj_bytes += obj_size;
}

/*!
 * Generate a listing of what object files go in which dgos
 */
std::string ObjectFileDB::generate_dgo_listing() {
  std::string result = ";; DGO File Listing\n\n";
  std::vector<std::string> dgo_names;
  for(auto& kv : obj_files_by_dgo) {
    dgo_names.push_back(kv.first);
  }

  std::sort(dgo_names.begin(), dgo_names.end());

  for(const auto& name : dgo_names) {
    result += "(\"" + name + "\"\n";
    for(auto& obj : obj_files_by_dgo[name]) {
      result += "  " + obj.name + " :version " + std::to_string(obj.version) + "\n";
    }
    result += "  )\n\n";
  }

  return result;
}

/*!
 * Process all of the linking data of all objects.
 */
void ObjectFileDB::process_link_data() {
  printf("- Processing Link Data...\n");
  Timer process_link_timer;

  LinkedObjectFile::Stats combined_stats;

  for(auto& kv : obj_files_by_name) {
    for(auto& obj : kv.second) {
      obj.linked_data = to_linked_object_file(obj.data, obj.record.name);
      combined_stats.add(obj.linked_data.stats);
    }
  }

  printf("Processed Link Data:\n");
  printf(" code %d bytes\n", combined_stats.total_code_bytes);
  printf(" v2 code %d bytes\n", combined_stats.total_v2_code_bytes);
  printf(" v2 link data %d bytes\n", combined_stats.total_v2_link_bytes);
  printf(" v2 pointers %d\n", combined_stats.total_v2_pointers);
  printf(" v2 pointer seeks %d\n", combined_stats.total_v2_pointer_seeks);
  printf(" v2 symbols %d\n", combined_stats.total_v2_symbol_count);
  printf(" v2 symbol links %d\n", combined_stats.total_v2_symbol_links);

  printf(" v3 code %d bytes\n", combined_stats.v3_code_bytes);
  printf(" v3 link data %d bytes\n", combined_stats.v3_link_bytes);
  printf(" v3 pointers %d\n", combined_stats.v3_pointers);
  printf("   split %d\n", combined_stats.v3_split_pointers);
  printf("   word  %d\n", combined_stats.v3_word_pointers);
  printf(" v3 pointer seeks %d\n", combined_stats.v3_pointer_seeks);
  printf(" v3 symbols %d\n", combined_stats.v3_symbol_count);
  printf(" v3 offset symbol links %d\n", combined_stats.v3_symbol_link_offset);
  printf(" v3 word symbol links %d\n", combined_stats.v3_symbol_link_word);

  printf(" total %.3f ms\n", process_link_timer.getMs());
  printf("\n");
}

/*!
 * Process all of the labels generated from linking and give them reasonable names.
 */
void ObjectFileDB::process_labels() {
  printf("- Processing Labels...\n");
  Timer process_label_timer;
  uint32_t total = 0;
  for(auto& kv : obj_files_by_name) {
    for(auto& obj : kv.second) {
      total += obj.linked_data.set_ordered_label_names();
    }
  }

  printf("Processed Labels:\n");
  printf(" total %d labels\n", total);
  printf(" total %.3f ms\n", process_label_timer.getMs());
  printf("\n");
}

/*!
 * Dump object files and their linking data to text files for debugging
 */
void ObjectFileDB::write_object_file_words(const std::string &output_dir, bool dump_v3_only) {
  if(dump_v3_only) {
    printf("- Writing object file dumps (v3 only)...\n");
  } else {
    printf("- Writing object file dumps (all)...\n");
  }

  Timer timer;
  uint32_t total_bytes = 0, total_files = 0;

  for(auto& kv : obj_files_by_name) {
    for(auto& obj : kv.second) {
      if(obj.linked_data.segments == 3 || !dump_v3_only) {
        auto file_text = obj.linked_data.print_words();
        auto file_name = combine_path(output_dir, obj.record.to_unique_name() + ".txt");
        total_bytes += file_text.size();
        write_text_file(file_name, file_text);
        total_files++;
      }
    }
  }

  printf("Wrote object file dumps:\n");
  printf(" total %d files\n", total_files);
  printf(" total %.3f MB\n", total_bytes / ((float) (1u << 20u)));
  printf(" total %.3f ms (%.3f MB/sec)\n", timer.getMs(), total_bytes / ((1u << 20u) * timer.getSeconds()));
  printf("\n");
}

/*!
 * Dump disassembly for object files containing code.  Data zones will also be dumped.
 */
void ObjectFileDB::write_disassembly(const std::string &output_dir) {
  printf("- Writing functions...\n");
  Timer timer;
  uint32_t total_bytes = 0, total_files = 0;

  for(auto& kv : obj_files_by_name) {
    for(auto& obj : kv.second) {
      // for now, also dump objects without functions.
//      if(!obj.linked_data.has_any_functions()) continue;
      auto file_text = obj.linked_data.print_disassembly();

      auto file_name = combine_path(output_dir, obj.record.to_unique_name() + ".func");
      total_bytes += file_text.size();
      write_text_file(file_name, file_text);
      total_files++;
    }
  }

  printf("Wrote functions dumps:\n");
  printf(" total %d files\n", total_files);
  printf(" total %.3f MB\n", total_bytes / ((float) (1u << 20u)));
  printf(" total %.3f ms (%.3f MB/sec)\n", timer.getMs(), total_bytes / ((1u << 20u) * timer.getSeconds()));
  printf("\n");
}

/*!
 * Find code/data zones, identify functions, and disassemble
 */
void ObjectFileDB::find_code() {
  printf("- Finding code in object files...\n");
  LinkedObjectFile::Stats combined_stats;
  Timer timer;

  for(auto& kv : obj_files_by_name) {
    for(auto& obj : kv.second) {
//      printf("fc %s\n", obj.record.to_unique_name().c_str());
      obj.linked_data.find_code();
      obj.linked_data.find_functions();
      obj.linked_data.disassemble_functions();

      if(gGameVersion != JAK2 || obj.record.to_unique_name() != "effect-control-v0") {
        obj.linked_data.process_fp_relative_links();
      } else {
        printf("skipping process_fp_relative_links in %s\n", obj.record.to_unique_name().c_str());
      }


      auto& obj_stats = obj.linked_data.stats;
      if(obj_stats.code_bytes / 4 > obj_stats.decoded_ops) {
        printf("Failed to decode all in %s (%d / %d)\n", obj.record.to_unique_name().c_str(), obj_stats.decoded_ops, obj_stats.code_bytes / 4);
      }
      combined_stats.add(obj.linked_data.stats);
    }
  }

  printf("Found code:\n");
  printf(" code %.3f MB\n", combined_stats.code_bytes / (float)(1 << 20));
  printf(" data %.3f MB\n", combined_stats.data_bytes / (float)(1 << 20));
  printf(" functions: %d\n", combined_stats.function_count);
  printf(" fp uses resolved: %d / %d (%.3f %%)\n", combined_stats.n_fp_reg_use_resolved, combined_stats.n_fp_reg_use,
         100.f * (float)combined_stats.n_fp_reg_use_resolved / combined_stats.n_fp_reg_use);
  auto total_ops = combined_stats.code_bytes / 4;
  printf(" decoded %d / %d (%.3f %%)\n", combined_stats.decoded_ops, total_ops, 100.f * (float)combined_stats.decoded_ops / total_ops);
  printf(" total %.3f ms\n", timer.getMs());
  printf("\n");
}