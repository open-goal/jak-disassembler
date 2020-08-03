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
#include "config.h"
#include "third-party/minilzo/minilzo.h"
#include "util/BinaryReader.h"
#include "util/FileIO.h"
#include "util/Timer.h"
#include "Function/BasicBlocks.h"
#include "util/Log.h"

/*!
 * Get a unique name for this object file.
 */
std::string ObjectFileRecord::to_unique_name() const {
  return name + "-v" + std::to_string(version);
}

/*!
 * Build an object file DB for the given list of DGOs.
 */
ObjectFileDB::ObjectFileDB(const std::vector<std::string>& _dgos) {
  Timer timer;

  logger.writeln("- Initializing ObjectFileDB...");
  for (auto& dgo : _dgos) {
    get_objs_from_dgo(dgo);
  }

  logger.writeln("ObjectFileDB Initialized:");
  logger.writeln(" total dgos: %ld", _dgos.size());
  logger.writeln(" total data: %d bytes", stats.total_dgo_bytes);
  logger.writeln(" total objs: %d", stats.total_obj_files);
  logger.writeln(" unique objs: %d", stats.unique_obj_files);
  logger.writeln(" unique data: %d bytes", stats.unique_obj_bytes);
  logger.writeln(" total %.1f ms (%.3f MB/sec, %.3f obj/sec)", timer.getMs(),
                 stats.total_dgo_bytes / ((1u << 20u) * timer.getSeconds()),
                 stats.total_obj_files / timer.getSeconds());
  logger.writeln("");
}

// Header for a DGO file
struct DgoHeader {
  uint32_t size;
  char name[60];
};

namespace {
/*!
 * Assert false if the char[] has non-null data after the null terminated string.
 * Used to sanity check the sizes of strings in DGO/object file headers.
 */
void assert_string_empty_after(const char* str, int size) {
  auto ptr = str;
  while (*ptr)
    ptr++;
  while (ptr - str < size) {
    assert(!*ptr);
    ptr++;
  }
}
}  // namespace

constexpr int MAX_CHUNK_SIZE = 0x8000;
/*!
 * Load the objects stored in the given DGO into the ObjectFileDB
 */
void ObjectFileDB::get_objs_from_dgo(const std::string& filename) {
  auto dgo_data = read_binary_file(filename);
  stats.total_dgo_bytes += dgo_data.size();

  const char jak2_header[] = "oZlB";
  bool is_jak2 = true;
  for (int i = 0; i < 4; i++) {
    if (jak2_header[i] != dgo_data[i]) {
      is_jak2 = false;
    }
  }

  if (is_jak2) {
    if (lzo_init() != LZO_E_OK) {
      assert(false);
    }
    BinaryReader compressed_reader(dgo_data);
    // seek past oZlB
    compressed_reader.ffwd(4);
    auto decompressed_size = compressed_reader.read<uint32_t>();
    std::vector<uint8_t> decompressed_data;
    decompressed_data.resize(decompressed_size);
    size_t output_offset = 0;
    while (true) {
      // seek past alignment bytes and read the next chunk size
      uint32_t chunk_size = 0;
      while (!chunk_size) {
        chunk_size = compressed_reader.read<uint32_t>();
      }

      if (chunk_size < MAX_CHUNK_SIZE) {
        lzo_uint bytes_written;
        auto lzo_rv =
            lzo1x_decompress(compressed_reader.here(), chunk_size,
                             decompressed_data.data() + output_offset, &bytes_written, nullptr);
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

      if (output_offset >= decompressed_size)
        break;
      while (compressed_reader.get_seek() % 4) {
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
  for (uint32_t i = 0; i < header.size; i++) {
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
void ObjectFileDB::add_obj_from_dgo(const std::string& obj_name,
                                    uint8_t* obj_data,
                                    uint32_t obj_size,
                                    const std::string& dgo_name) {
  stats.total_obj_files++;

  auto hash = crc32(obj_data, obj_size);

  // first, check to see if we already got it...
  for (auto& e : obj_files_by_name[obj_name]) {
    if (e.data.size() == obj_size && e.record.hash == hash) {
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
  if (obj_files_by_name[obj_name].empty()) {
    // if this is the first time we've seen this object file name, add it in the order.
    obj_file_order.push_back(obj_name);
  }
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
  for (auto& kv : obj_files_by_dgo) {
    dgo_names.push_back(kv.first);
  }

  std::sort(dgo_names.begin(), dgo_names.end());

  for (const auto& name : dgo_names) {
    result += "(\"" + name + "\"\n";
    for (auto& obj : obj_files_by_dgo[name]) {
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
  logger.writeln("- Processing Link Data...");
  Timer process_link_timer;

  LinkedObjectFile::Stats combined_stats;

  for_each_obj([&](ObjectFileData& obj) {
    obj.linked_data = to_linked_object_file(obj.data, obj.record.name);
    combined_stats.add(obj.linked_data.stats);
  });

  logger.writeln("Processed Link Data:");
  logger.writeln(" code %d bytes", combined_stats.total_code_bytes);
  logger.writeln(" v2 code %d bytes", combined_stats.total_v2_code_bytes);
  logger.writeln(" v2 link data %d bytes", combined_stats.total_v2_link_bytes);
  logger.writeln(" v2 pointers %d", combined_stats.total_v2_pointers);
  logger.writeln(" v2 pointer seeks %d", combined_stats.total_v2_pointer_seeks);
  logger.writeln(" v2 symbols %d", combined_stats.total_v2_symbol_count);
  logger.writeln(" v2 symbol links %d", combined_stats.total_v2_symbol_links);

  logger.writeln(" v3 code %d bytes", combined_stats.v3_code_bytes);
  logger.writeln(" v3 link data %d bytes", combined_stats.v3_link_bytes);
  logger.writeln(" v3 pointers %d", combined_stats.v3_pointers);
  logger.writeln("   split %d", combined_stats.v3_split_pointers);
  logger.writeln("   word  %d", combined_stats.v3_word_pointers);
  logger.writeln(" v3 pointer seeks %d", combined_stats.v3_pointer_seeks);
  logger.writeln(" v3 symbols %d", combined_stats.v3_symbol_count);
  logger.writeln(" v3 offset symbol links %d", combined_stats.v3_symbol_link_offset);
  logger.writeln(" v3 word symbol links %d", combined_stats.v3_symbol_link_word);

  logger.writeln(" total %.3f ms", process_link_timer.getMs());
  logger.writeln("");
}

/*!
 * Process all of the labels generated from linking and give them reasonable names.
 */
void ObjectFileDB::process_labels() {
  logger.writeln("- Processing Labels...");
  Timer process_label_timer;
  uint32_t total = 0;
  for_each_obj([&](ObjectFileData& obj) { total += obj.linked_data.set_ordered_label_names(); });

  logger.writeln("Processed Labels:");
  logger.writeln(" total %d labels", total);
  logger.writeln(" total %.3f ms", process_label_timer.getMs());
  logger.writeln("");
}

/*!
 * Dump object files and their linking data to text files for debugging
 */
void ObjectFileDB::write_object_file_words(const std::string& output_dir, bool dump_v3_only) {
  if (dump_v3_only) {
    logger.writeln("- Writing object file dumps (v3 only)...");
  } else {
    logger.writeln("- Writing object file dumps (all)...");
  }

  Timer timer;
  uint32_t total_bytes = 0, total_files = 0;

  for_each_obj([&](ObjectFileData& obj) {
    if (obj.linked_data.segments == 3 || !dump_v3_only) {
      auto file_text = obj.linked_data.print_words();
      auto file_name = combine_path(output_dir, obj.record.to_unique_name() + ".txt");
      total_bytes += file_text.size();
      write_text_file(file_name, file_text);
      total_files++;
    }
  });

  logger.writeln("Wrote object file dumps:");
  logger.writeln(" total %d files", total_files);
  logger.writeln(" total %.3f MB", total_bytes / ((float)(1u << 20u)));
  logger.writeln(" total %.3f ms (%.3f MB/sec)", timer.getMs(),
                 total_bytes / ((1u << 20u) * timer.getSeconds()));
  logger.writeln("");
}

/*!
 * Dump disassembly for object files containing code.  Data zones will also be dumped.
 */
void ObjectFileDB::write_disassembly(const std::string& output_dir,
                                     bool disassemble_objects_without_functions) {
  logger.writeln("- Writing functions...");
  Timer timer;
  uint32_t total_bytes = 0, total_files = 0;

  for_each_obj([&](ObjectFileData& obj) {
    if (obj.linked_data.has_any_functions() || disassemble_objects_without_functions) {
      auto file_text = obj.linked_data.print_disassembly();
      auto file_name = combine_path(output_dir, obj.record.to_unique_name() + ".func");
      total_bytes += file_text.size();
      write_text_file(file_name, file_text);
      total_files++;
    }
  });

  logger.writeln("Wrote functions dumps:");
  logger.writeln(" total %d files", total_files);
  logger.writeln(" total %.3f MB", total_bytes / ((float)(1u << 20u)));
  logger.writeln(" total %.3f ms (%.3f MB/sec)", timer.getMs(),
                 total_bytes / ((1u << 20u) * timer.getSeconds()));
  logger.writeln("");
}

/*!
 * Find code/data zones, identify functions, and disassemble
 */
void ObjectFileDB::find_code() {
  logger.writeln("- Finding code in object files...");
  LinkedObjectFile::Stats combined_stats;
  Timer timer;

  for_each_obj([&](ObjectFileData& obj) {
    //      printf("fc %s\n", obj.record.to_unique_name().c_str());
    obj.linked_data.find_code();
    obj.linked_data.find_functions();
    obj.linked_data.disassemble_functions();

    if (get_config().game_version == 1 || obj.record.to_unique_name() != "effect-control-v0") {
      obj.linked_data.process_fp_relative_links();
    } else {
      logger.writeln("skipping process_fp_relative_links in %s",
                     obj.record.to_unique_name().c_str());
    }

    auto& obj_stats = obj.linked_data.stats;
    if (obj_stats.code_bytes / 4 > obj_stats.decoded_ops) {
      logger.writeln("Failed to decode all in %s (%d / %d)", obj.record.to_unique_name().c_str(),
                     obj_stats.decoded_ops, obj_stats.code_bytes / 4);
    }
    combined_stats.add(obj.linked_data.stats);
  });

  logger.writeln("Found code:");
  logger.writeln(" code %.3f MB", combined_stats.code_bytes / (float)(1 << 20));
  logger.writeln(" data %.3f MB", combined_stats.data_bytes / (float)(1 << 20));
  logger.writeln(" functions: %d", combined_stats.function_count);
  logger.writeln(" fp uses resolved: %d / %d (%.3f %%)", combined_stats.n_fp_reg_use_resolved,
                 combined_stats.n_fp_reg_use,
                 100.f * (float)combined_stats.n_fp_reg_use_resolved / combined_stats.n_fp_reg_use);
  auto total_ops = combined_stats.code_bytes / 4;
  logger.writeln(" decoded %d / %d (%.3f %%)", combined_stats.decoded_ops, total_ops,
                 100.f * (float)combined_stats.decoded_ops / total_ops);
  logger.writeln(" total %.3f ms", timer.getMs());
  logger.writeln("");
}

/*!
 * Finds and writes all scripts into a file named all_scripts.lisp.
 * Doesn't change any state in ObjectFileDB.
 */
void ObjectFileDB::find_and_write_scripts(const std::string& output_dir) {
  logger.writeln("- Finding scripts in object files...");
  Timer timer;
  std::string all_scripts;

  for_each_obj([&](ObjectFileData& obj) {
    auto scripts = obj.linked_data.print_scripts();
    if (!scripts.empty()) {
      all_scripts += ";--------------------------------------\n";
      all_scripts += "; " + obj.record.to_unique_name() + "\n";
      all_scripts += ";---------------------------------------\n";
      all_scripts += scripts;
    }
  });

  auto file_name = combine_path(output_dir, "all_scripts.lisp");
  write_text_file(file_name, all_scripts);

  logger.writeln("Found scripts:");
  logger.writeln(" total %.3f ms", timer.getMs());
  logger.writeln("");
}

void ObjectFileDB::analyze_functions() {
  logger.writeln("- Analyzing Functions...");
  Timer timer;

  if (get_config().find_basic_blocks) {
    timer.start();
    int total_basic_blocks = 0;
    for_each_function([&](Function& func, int segment_id, ObjectFileData& data) {
      auto blocks = find_blocks_in_function(data.linked_data, segment_id, func);
      total_basic_blocks += blocks.size();
      func.basic_blocks = blocks;
      func.analyze_prologue(data.linked_data);
    });

    logger.writeln("Found %d basic blocks in %.3f ms", total_basic_blocks, timer.getMs());
  }

  {
    timer.start();
    for_each_obj([&](ObjectFileData& data) {
      if (data.linked_data.segments == 3) {
        // the top level segment should have a single function
        assert(data.linked_data.functions_by_seg.at(2).size() == 1);

        auto& func = data.linked_data.functions_by_seg.at(2).front();
        assert(func.guessed_name.empty());
        func.guessed_name = "(top-level-init)";
        func.find_global_function_defs(data.linked_data);
      }
    });
  }
}
