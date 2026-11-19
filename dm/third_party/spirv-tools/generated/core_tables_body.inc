
IndexRange ExtensionToIndexRange(Extension extension) {
  switch(extension) {
    case Extension::kSPV_ALTERA_arbitrary_precision_fixed_point: return IR(0, 43);
    case Extension::kSPV_ALTERA_arbitrary_precision_floating_point: return IR(43, 46);
    case Extension::kSPV_ALTERA_arbitrary_precision_integers: return IR(89, 40);
    case Extension::kSPV_ALTERA_blocking_pipes: return IR(129, 26);
    case Extension::kSPV_ALTERA_fpga_argument_interfaces: return IR(155, 36);
    case Extension::kSPV_ALTERA_fpga_buffer_location: return IR(191, 32);
    case Extension::kSPV_ALTERA_fpga_cluster_attributes: return IR(223, 35);
    case Extension::kSPV_ALTERA_fpga_dsp_control: return IR(258, 28);
    case Extension::kSPV_ALTERA_fpga_invocation_pipelining_attributes: return IR(286, 49);
    case Extension::kSPV_ALTERA_fpga_latency_control: return IR(335, 32);
    case Extension::kSPV_ALTERA_fpga_loop_controls: return IR(367, 30);
    case Extension::kSPV_ALTERA_fpga_memory_accesses: return IR(397, 32);
    case Extension::kSPV_ALTERA_fpga_memory_attributes: return IR(429, 34);
    case Extension::kSPV_ALTERA_fpga_reg: return IR(463, 20);
    case Extension::kSPV_ALTERA_global_variable_fpga_decorations: return IR(483, 44);
    case Extension::kSPV_ALTERA_io_pipes: return IR(527, 20);
    case Extension::kSPV_ALTERA_loop_fuse: return IR(547, 21);
    case Extension::kSPV_ALTERA_runtime_aligned: return IR(568, 27);
    case Extension::kSPV_ALTERA_task_sequence: return IR(595, 25);
    case Extension::kSPV_ALTERA_usm_storage_classes: return IR(620, 31);
    case Extension::kSPV_AMDX_shader_enqueue: return IR(651, 24);
    case Extension::kSPV_AMD_gcn_shader: return IR(675, 19);
    case Extension::kSPV_AMD_gpu_shader_half_float: return IR(694, 30);
    case Extension::kSPV_AMD_gpu_shader_half_float_fetch: return IR(724, 36);
    case Extension::kSPV_AMD_gpu_shader_int16: return IR(760, 25);
    case Extension::kSPV_AMD_shader_ballot: return IR(785, 22);
    case Extension::kSPV_AMD_shader_early_and_late_fragment_tests: return IR(807, 45);
    case Extension::kSPV_AMD_shader_explicit_vertex_parameter: return IR(852, 41);
    case Extension::kSPV_AMD_shader_fragment_mask: return IR(893, 29);
    case Extension::kSPV_AMD_shader_image_load_store_lod: return IR(922, 36);
    case Extension::kSPV_AMD_shader_trinary_minmax: return IR(958, 30);
    case Extension::kSPV_AMD_texture_gather_bias_lod: return IR(988, 32);
    case Extension::kSPV_AMD_weak_linkage: return IR(1020, 21);
    case Extension::kSPV_ARM_cooperative_matrix_layouts: return IR(1041, 35);
    case Extension::kSPV_ARM_core_builtins: return IR(1076, 22);
    case Extension::kSPV_ARM_graph: return IR(1098, 14);
    case Extension::kSPV_ARM_tensors: return IR(1112, 16);
    case Extension::kSPV_EXT_arithmetic_fence: return IR(1128, 25);
    case Extension::kSPV_EXT_demote_to_helper_invocation: return IR(1153, 36);
    case Extension::kSPV_EXT_descriptor_heap: return IR(1189, 24);
    case Extension::kSPV_EXT_descriptor_indexing: return IR(1213, 28);
    case Extension::kSPV_EXT_float8: return IR(1241, 15);
    case Extension::kSPV_EXT_fragment_fully_covered: return IR(1256, 31);
    case Extension::kSPV_EXT_fragment_invocation_density: return IR(1287, 36);
    case Extension::kSPV_EXT_fragment_shader_interlock: return IR(1323, 34);
    case Extension::kSPV_EXT_long_vector: return IR(1357, 20);
    case Extension::kSPV_EXT_mesh_shader: return IR(1377, 20);
    case Extension::kSPV_EXT_ocp_microscaling_types: return IR(1397, 31);
    case Extension::kSPV_EXT_opacity_micromap: return IR(1428, 25);
    case Extension::kSPV_EXT_optnone: return IR(1453, 16);
    case Extension::kSPV_EXT_physical_storage_buffer: return IR(1469, 32);
    case Extension::kSPV_EXT_relaxed_printf_string_address_space: return IR(1501, 44);
    case Extension::kSPV_EXT_replicated_composites: return IR(1545, 30);
    case Extension::kSPV_EXT_shader_64bit_indexing: return IR(1575, 30);
    case Extension::kSPV_EXT_shader_atomic_float16_add: return IR(1605, 34);
    case Extension::kSPV_EXT_shader_atomic_float_add: return IR(1639, 32);
    case Extension::kSPV_EXT_shader_atomic_float_min_max: return IR(1671, 36);
    case Extension::kSPV_EXT_shader_image_int64: return IR(1707, 27);
    case Extension::kSPV_EXT_shader_invocation_reorder: return IR(1734, 34);
    case Extension::kSPV_EXT_shader_stencil_export: return IR(1768, 30);
    case Extension::kSPV_EXT_shader_subgroup_partitioned: return IR(1798, 36);
    case Extension::kSPV_EXT_shader_tile_image: return IR(1834, 26);
    case Extension::kSPV_EXT_shader_viewport_index_layer: return IR(1860, 36);
    case Extension::kSPV_EXT_split_barrier: return IR(1896, 22);
    case Extension::kSPV_GOOGLE_decorate_string: return IR(1918, 27);
    case Extension::kSPV_GOOGLE_hlsl_functionality1: return IR(1945, 31);
    case Extension::kSPV_GOOGLE_user_type: return IR(1976, 21);
    case Extension::kSPV_INTEL_2d_block_io: return IR(1997, 22);
    case Extension::kSPV_INTEL_arbitrary_precision_fixed_point: return IR(2019, 42);
    case Extension::kSPV_INTEL_arbitrary_precision_floating_point: return IR(2061, 45);
    case Extension::kSPV_INTEL_arbitrary_precision_integers: return IR(2106, 39);
    case Extension::kSPV_INTEL_bfloat16_conversion: return IR(2145, 30);
    case Extension::kSPV_INTEL_bindless_images: return IR(2175, 26);
    case Extension::kSPV_INTEL_blocking_pipes: return IR(2201, 25);
    case Extension::kSPV_INTEL_cache_controls: return IR(2226, 25);
    case Extension::kSPV_INTEL_debug_module: return IR(2251, 23);
    case Extension::kSPV_INTEL_device_side_avc_motion_estimation: return IR(2274, 44);
    case Extension::kSPV_INTEL_float_controls2: return IR(2318, 26);
    case Extension::kSPV_INTEL_fp_fast_math_mode: return IR(2344, 28);
    case Extension::kSPV_INTEL_fp_max_error: return IR(2372, 23);
    case Extension::kSPV_INTEL_fpga_argument_interfaces: return IR(2395, 35);
    case Extension::kSPV_INTEL_fpga_buffer_location: return IR(2430, 31);
    case Extension::kSPV_INTEL_fpga_cluster_attributes: return IR(2461, 34);
    case Extension::kSPV_INTEL_fpga_dsp_control: return IR(2495, 27);
    case Extension::kSPV_INTEL_fpga_invocation_pipelining_attributes: return IR(2522, 48);
    case Extension::kSPV_INTEL_fpga_latency_control: return IR(2570, 31);
    case Extension::kSPV_INTEL_fpga_loop_controls: return IR(2601, 29);
    case Extension::kSPV_INTEL_fpga_memory_accesses: return IR(2630, 31);
    case Extension::kSPV_INTEL_fpga_memory_attributes: return IR(2661, 33);
    case Extension::kSPV_INTEL_fpga_reg: return IR(2694, 19);
    case Extension::kSPV_INTEL_function_pointers: return IR(2713, 28);
    case Extension::kSPV_INTEL_function_variants: return IR(2741, 28);
    case Extension::kSPV_INTEL_global_variable_fpga_decorations: return IR(2769, 43);
    case Extension::kSPV_INTEL_global_variable_host_access: return IR(2812, 38);
    case Extension::kSPV_INTEL_inline_assembly: return IR(2850, 26);
    case Extension::kSPV_INTEL_int4: return IR(2876, 15);
    case Extension::kSPV_INTEL_io_pipes: return IR(2891, 19);
    case Extension::kSPV_INTEL_kernel_attributes: return IR(2910, 28);
    case Extension::kSPV_INTEL_long_composites: return IR(2938, 26);
    case Extension::kSPV_INTEL_loop_fuse: return IR(2964, 20);
    case Extension::kSPV_INTEL_masked_gather_scatter: return IR(2984, 32);
    case Extension::kSPV_INTEL_maximum_registers: return IR(3016, 28);
    case Extension::kSPV_INTEL_media_block_io: return IR(3044, 25);
    case Extension::kSPV_INTEL_memory_access_aliasing: return IR(3069, 33);
    case Extension::kSPV_INTEL_optnone: return IR(3102, 18);
    case Extension::kSPV_INTEL_predicated_io: return IR(3120, 24);
    case Extension::kSPV_INTEL_rounded_divide_sqrt: return IR(3144, 30);
    case Extension::kSPV_INTEL_runtime_aligned: return IR(3174, 26);
    case Extension::kSPV_INTEL_shader_integer_functions2: return IR(3200, 36);
    case Extension::kSPV_INTEL_split_barrier: return IR(3236, 24);
    case Extension::kSPV_INTEL_subgroup_buffer_prefetch: return IR(3260, 35);
    case Extension::kSPV_INTEL_subgroup_matrix_multiply_accumulate: return IR(3295, 46);
    case Extension::kSPV_INTEL_subgroups: return IR(3341, 20);
    case Extension::kSPV_INTEL_task_sequence: return IR(3361, 24);
    case Extension::kSPV_INTEL_tensor_float32_conversion: return IR(3385, 36);
    case Extension::kSPV_INTEL_ternary_bitwise_function: return IR(3421, 35);
    case Extension::kSPV_INTEL_unstructured_loop_controls: return IR(3456, 37);
    case Extension::kSPV_INTEL_usm_storage_classes: return IR(3493, 30);
    case Extension::kSPV_INTEL_variable_length_array: return IR(3523, 32);
    case Extension::kSPV_INTEL_vector_compute: return IR(3555, 25);
    case Extension::kSPV_KHR_16bit_storage: return IR(3580, 22);
    case Extension::kSPV_KHR_8bit_storage: return IR(3602, 21);
    case Extension::kSPV_KHR_abort: return IR(3623, 14);
    case Extension::kSPV_KHR_bfloat16: return IR(3637, 17);
    case Extension::kSPV_KHR_bit_instructions: return IR(3654, 25);
    case Extension::kSPV_KHR_compute_shader_derivatives: return IR(3679, 35);
    case Extension::kSPV_KHR_constant_data: return IR(3714, 22);
    case Extension::kSPV_KHR_cooperative_matrix: return IR(3736, 27);
    case Extension::kSPV_KHR_device_group: return IR(3763, 21);
    case Extension::kSPV_KHR_expect_assume: return IR(3784, 22);
    case Extension::kSPV_KHR_float_controls: return IR(3806, 23);
    case Extension::kSPV_KHR_float_controls2: return IR(3829, 24);
    case Extension::kSPV_KHR_fma: return IR(3853, 12);
    case Extension::kSPV_KHR_fragment_shader_barycentric: return IR(3865, 36);
    case Extension::kSPV_KHR_fragment_shading_rate: return IR(3901, 30);
    case Extension::kSPV_KHR_integer_dot_product: return IR(3931, 28);
    case Extension::kSPV_KHR_linkonce_odr: return IR(3959, 21);
    case Extension::kSPV_KHR_maximal_reconvergence: return IR(3980, 30);
    case Extension::kSPV_KHR_multiview: return IR(4010, 18);
    case Extension::kSPV_KHR_no_integer_wrap_decoration: return IR(4028, 35);
    case Extension::kSPV_KHR_non_semantic_info: return IR(4063, 26);
    case Extension::kSPV_KHR_opacity_micromap: return IR(4089, 25);
    case Extension::kSPV_KHR_physical_storage_buffer: return IR(4114, 32);
    case Extension::kSPV_KHR_poison_freeze: return IR(4146, 22);
    case Extension::kSPV_KHR_post_depth_coverage: return IR(4168, 28);
    case Extension::kSPV_KHR_quad_control: return IR(4196, 21);
    case Extension::kSPV_KHR_ray_cull_mask: return IR(4217, 22);
    case Extension::kSPV_KHR_ray_query: return IR(4239, 18);
    case Extension::kSPV_KHR_ray_tracing: return IR(4257, 20);
    case Extension::kSPV_KHR_ray_tracing_position_fetch: return IR(4277, 35);
    case Extension::kSPV_KHR_relaxed_extended_instruction: return IR(4312, 37);
    case Extension::kSPV_KHR_shader_atomic_counter_ops: return IR(4349, 34);
    case Extension::kSPV_KHR_shader_ballot: return IR(4383, 22);
    case Extension::kSPV_KHR_shader_clock: return IR(4405, 21);
    case Extension::kSPV_KHR_shader_draw_parameters: return IR(4426, 31);
    case Extension::kSPV_KHR_storage_buffer_storage_class: return IR(4457, 37);
    case Extension::kSPV_KHR_subgroup_rotate: return IR(4494, 24);
    case Extension::kSPV_KHR_subgroup_uniform_control_flow: return IR(4518, 38);
    case Extension::kSPV_KHR_subgroup_vote: return IR(4556, 22);
    case Extension::kSPV_KHR_terminate_invocation: return IR(4578, 29);
    case Extension::kSPV_KHR_uniform_group_instructions: return IR(4607, 35);
    case Extension::kSPV_KHR_untyped_pointers: return IR(4642, 25);
    case Extension::kSPV_KHR_variable_pointers: return IR(4667, 26);
    case Extension::kSPV_KHR_vulkan_memory_model: return IR(4693, 28);
    case Extension::kSPV_KHR_workgroup_memory_explicit_layout: return IR(4721, 41);
    case Extension::kSPV_NVX_multiview_per_view_attributes: return IR(4762, 38);
    case Extension::kSPV_NV_bindless_texture: return IR(4800, 24);
    case Extension::kSPV_NV_cluster_acceleration_structure: return IR(4824, 38);
    case Extension::kSPV_NV_compute_shader_derivatives: return IR(4862, 34);
    case Extension::kSPV_NV_cooperative_matrix: return IR(4896, 26);
    case Extension::kSPV_NV_cooperative_matrix2: return IR(4922, 27);
    case Extension::kSPV_NV_cooperative_matrix_decode_vector: return IR(4949, 40);
    case Extension::kSPV_NV_cooperative_vector: return IR(4989, 26);
    case Extension::kSPV_NV_displacement_micromap: return IR(5015, 29);
    case Extension::kSPV_NV_fragment_shader_barycentric: return IR(5044, 35);
    case Extension::kSPV_NV_geometry_shader_passthrough: return IR(5079, 35);
    case Extension::kSPV_NV_linear_swept_spheres: return IR(5114, 28);
    case Extension::kSPV_NV_mesh_shader: return IR(5142, 19);
    case Extension::kSPV_NV_push_constant_bank: return IR(5161, 26);
    case Extension::kSPV_NV_raw_access_chains: return IR(5187, 25);
    case Extension::kSPV_NV_ray_tracing: return IR(5212, 19);
    case Extension::kSPV_NV_ray_tracing_motion_blur: return IR(5231, 31);
    case Extension::kSPV_NV_sample_mask_override_coverage: return IR(5262, 37);
    case Extension::kSPV_NV_shader_atomic_fp16_vector: return IR(5299, 33);
    case Extension::kSPV_NV_shader_image_footprint: return IR(5332, 30);
    case Extension::kSPV_NV_shader_invocation_reorder: return IR(5362, 33);
    case Extension::kSPV_NV_shader_sm_builtins: return IR(5395, 26);
    case Extension::kSPV_NV_shader_subgroup_partitioned: return IR(5421, 35);
    case Extension::kSPV_NV_shading_rate: return IR(5456, 20);
    case Extension::kSPV_NV_stereo_view_rendering: return IR(5476, 29);
    case Extension::kSPV_NV_tensor_addressing: return IR(5505, 25);
    case Extension::kSPV_NV_viewport_array2: return IR(5530, 23);
    case Extension::kSPV_QCOM_cooperative_matrix_conversion: return IR(5553, 39);
    case Extension::kSPV_QCOM_image_processing: return IR(5592, 26);
    case Extension::kSPV_QCOM_image_processing2: return IR(5618, 27);
    case Extension::kSPV_QCOM_image_processing3: return IR(5645, 27);
    case Extension::kSPV_QCOM_multiple_wait_queues: return IR(5672, 30);
    case Extension::kSPV_QCOM_tile_shading: return IR(5702, 22);
    case Extension::kSPV_SAMSUNG_intrinsic: return IR(5724, 22);
    case Extension::kSPV_VALIDATOR_ignore_type_decl_unique: return IR(5746, 38);
    case Extension::kSPV_VALVE_mixed_float_dot_product: return IR(5784, 34);
    default: break;
  }
  return {};
}





static const std::array<NameValue,201> kExtensionNames{{
    {IR(0, 43), static_cast<uint32_t>(kSPV_ALTERA_arbitrary_precision_fixed_point)},
    {IR(43, 46), static_cast<uint32_t>(kSPV_ALTERA_arbitrary_precision_floating_point)},
    {IR(89, 40), static_cast<uint32_t>(kSPV_ALTERA_arbitrary_precision_integers)},
    {IR(129, 26), static_cast<uint32_t>(kSPV_ALTERA_blocking_pipes)},
    {IR(155, 36), static_cast<uint32_t>(kSPV_ALTERA_fpga_argument_interfaces)},
    {IR(191, 32), static_cast<uint32_t>(kSPV_ALTERA_fpga_buffer_location)},
    {IR(223, 35), static_cast<uint32_t>(kSPV_ALTERA_fpga_cluster_attributes)},
    {IR(258, 28), static_cast<uint32_t>(kSPV_ALTERA_fpga_dsp_control)},
    {IR(286, 49), static_cast<uint32_t>(kSPV_ALTERA_fpga_invocation_pipelining_attributes)},
    {IR(335, 32), static_cast<uint32_t>(kSPV_ALTERA_fpga_latency_control)},
    {IR(367, 30), static_cast<uint32_t>(kSPV_ALTERA_fpga_loop_controls)},
    {IR(397, 32), static_cast<uint32_t>(kSPV_ALTERA_fpga_memory_accesses)},
    {IR(429, 34), static_cast<uint32_t>(kSPV_ALTERA_fpga_memory_attributes)},
    {IR(463, 20), static_cast<uint32_t>(kSPV_ALTERA_fpga_reg)},
    {IR(483, 44), static_cast<uint32_t>(kSPV_ALTERA_global_variable_fpga_decorations)},
    {IR(527, 20), static_cast<uint32_t>(kSPV_ALTERA_io_pipes)},
    {IR(547, 21), static_cast<uint32_t>(kSPV_ALTERA_loop_fuse)},
    {IR(568, 27), static_cast<uint32_t>(kSPV_ALTERA_runtime_aligned)},
    {IR(595, 25), static_cast<uint32_t>(kSPV_ALTERA_task_sequence)},
    {IR(620, 31), static_cast<uint32_t>(kSPV_ALTERA_usm_storage_classes)},
    {IR(651, 24), static_cast<uint32_t>(kSPV_AMDX_shader_enqueue)},
    {IR(675, 19), static_cast<uint32_t>(kSPV_AMD_gcn_shader)},
    {IR(694, 30), static_cast<uint32_t>(kSPV_AMD_gpu_shader_half_float)},
    {IR(724, 36), static_cast<uint32_t>(kSPV_AMD_gpu_shader_half_float_fetch)},
    {IR(760, 25), static_cast<uint32_t>(kSPV_AMD_gpu_shader_int16)},
    {IR(785, 22), static_cast<uint32_t>(kSPV_AMD_shader_ballot)},
    {IR(807, 45), static_cast<uint32_t>(kSPV_AMD_shader_early_and_late_fragment_tests)},
    {IR(852, 41), static_cast<uint32_t>(kSPV_AMD_shader_explicit_vertex_parameter)},
    {IR(893, 29), static_cast<uint32_t>(kSPV_AMD_shader_fragment_mask)},
    {IR(922, 36), static_cast<uint32_t>(kSPV_AMD_shader_image_load_store_lod)},
    {IR(958, 30), static_cast<uint32_t>(kSPV_AMD_shader_trinary_minmax)},
    {IR(988, 32), static_cast<uint32_t>(kSPV_AMD_texture_gather_bias_lod)},
    {IR(1020, 21), static_cast<uint32_t>(kSPV_AMD_weak_linkage)},
    {IR(1041, 35), static_cast<uint32_t>(kSPV_ARM_cooperative_matrix_layouts)},
    {IR(1076, 22), static_cast<uint32_t>(kSPV_ARM_core_builtins)},
    {IR(1098, 14), static_cast<uint32_t>(kSPV_ARM_graph)},
    {IR(1112, 16), static_cast<uint32_t>(kSPV_ARM_tensors)},
    {IR(1128, 25), static_cast<uint32_t>(kSPV_EXT_arithmetic_fence)},
    {IR(1153, 36), static_cast<uint32_t>(kSPV_EXT_demote_to_helper_invocation)},
    {IR(1189, 24), static_cast<uint32_t>(kSPV_EXT_descriptor_heap)},
    {IR(1213, 28), static_cast<uint32_t>(kSPV_EXT_descriptor_indexing)},
    {IR(1241, 15), static_cast<uint32_t>(kSPV_EXT_float8)},
    {IR(1256, 31), static_cast<uint32_t>(kSPV_EXT_fragment_fully_covered)},
    {IR(1287, 36), static_cast<uint32_t>(kSPV_EXT_fragment_invocation_density)},
    {IR(1323, 34), static_cast<uint32_t>(kSPV_EXT_fragment_shader_interlock)},
    {IR(1357, 20), static_cast<uint32_t>(kSPV_EXT_long_vector)},
    {IR(1377, 20), static_cast<uint32_t>(kSPV_EXT_mesh_shader)},
    {IR(1397, 31), static_cast<uint32_t>(kSPV_EXT_ocp_microscaling_types)},
    {IR(1428, 25), static_cast<uint32_t>(kSPV_EXT_opacity_micromap)},
    {IR(1453, 16), static_cast<uint32_t>(kSPV_EXT_optnone)},
    {IR(1469, 32), static_cast<uint32_t>(kSPV_EXT_physical_storage_buffer)},
    {IR(1501, 44), static_cast<uint32_t>(kSPV_EXT_relaxed_printf_string_address_space)},
    {IR(1545, 30), static_cast<uint32_t>(kSPV_EXT_replicated_composites)},
    {IR(1575, 30), static_cast<uint32_t>(kSPV_EXT_shader_64bit_indexing)},
    {IR(1605, 34), static_cast<uint32_t>(kSPV_EXT_shader_atomic_float16_add)},
    {IR(1639, 32), static_cast<uint32_t>(kSPV_EXT_shader_atomic_float_add)},
    {IR(1671, 36), static_cast<uint32_t>(kSPV_EXT_shader_atomic_float_min_max)},
    {IR(1707, 27), static_cast<uint32_t>(kSPV_EXT_shader_image_int64)},
    {IR(1734, 34), static_cast<uint32_t>(kSPV_EXT_shader_invocation_reorder)},
    {IR(1768, 30), static_cast<uint32_t>(kSPV_EXT_shader_stencil_export)},
    {IR(1798, 36), static_cast<uint32_t>(kSPV_EXT_shader_subgroup_partitioned)},
    {IR(1834, 26), static_cast<uint32_t>(kSPV_EXT_shader_tile_image)},
    {IR(1860, 36), static_cast<uint32_t>(kSPV_EXT_shader_viewport_index_layer)},
    {IR(1896, 22), static_cast<uint32_t>(kSPV_EXT_split_barrier)},
    {IR(1918, 27), static_cast<uint32_t>(kSPV_GOOGLE_decorate_string)},
    {IR(1945, 31), static_cast<uint32_t>(kSPV_GOOGLE_hlsl_functionality1)},
    {IR(1976, 21), static_cast<uint32_t>(kSPV_GOOGLE_user_type)},
    {IR(1997, 22), static_cast<uint32_t>(kSPV_INTEL_2d_block_io)},
    {IR(2019, 42), static_cast<uint32_t>(kSPV_INTEL_arbitrary_precision_fixed_point)},
    {IR(2061, 45), static_cast<uint32_t>(kSPV_INTEL_arbitrary_precision_floating_point)},
    {IR(2106, 39), static_cast<uint32_t>(kSPV_INTEL_arbitrary_precision_integers)},
    {IR(2145, 30), static_cast<uint32_t>(kSPV_INTEL_bfloat16_conversion)},
    {IR(2175, 26), static_cast<uint32_t>(kSPV_INTEL_bindless_images)},
    {IR(2201, 25), static_cast<uint32_t>(kSPV_INTEL_blocking_pipes)},
    {IR(2226, 25), static_cast<uint32_t>(kSPV_INTEL_cache_controls)},
    {IR(2251, 23), static_cast<uint32_t>(kSPV_INTEL_debug_module)},
    {IR(2274, 44), static_cast<uint32_t>(kSPV_INTEL_device_side_avc_motion_estimation)},
    {IR(2318, 26), static_cast<uint32_t>(kSPV_INTEL_float_controls2)},
    {IR(2344, 28), static_cast<uint32_t>(kSPV_INTEL_fp_fast_math_mode)},
    {IR(2372, 23), static_cast<uint32_t>(kSPV_INTEL_fp_max_error)},
    {IR(2395, 35), static_cast<uint32_t>(kSPV_INTEL_fpga_argument_interfaces)},
    {IR(2430, 31), static_cast<uint32_t>(kSPV_INTEL_fpga_buffer_location)},
    {IR(2461, 34), static_cast<uint32_t>(kSPV_INTEL_fpga_cluster_attributes)},
    {IR(2495, 27), static_cast<uint32_t>(kSPV_INTEL_fpga_dsp_control)},
    {IR(2522, 48), static_cast<uint32_t>(kSPV_INTEL_fpga_invocation_pipelining_attributes)},
    {IR(2570, 31), static_cast<uint32_t>(kSPV_INTEL_fpga_latency_control)},
    {IR(2601, 29), static_cast<uint32_t>(kSPV_INTEL_fpga_loop_controls)},
    {IR(2630, 31), static_cast<uint32_t>(kSPV_INTEL_fpga_memory_accesses)},
    {IR(2661, 33), static_cast<uint32_t>(kSPV_INTEL_fpga_memory_attributes)},
    {IR(2694, 19), static_cast<uint32_t>(kSPV_INTEL_fpga_reg)},
    {IR(2713, 28), static_cast<uint32_t>(kSPV_INTEL_function_pointers)},
    {IR(2741, 28), static_cast<uint32_t>(kSPV_INTEL_function_variants)},
    {IR(2769, 43), static_cast<uint32_t>(kSPV_INTEL_global_variable_fpga_decorations)},
    {IR(2812, 38), static_cast<uint32_t>(kSPV_INTEL_global_variable_host_access)},
    {IR(2850, 26), static_cast<uint32_t>(kSPV_INTEL_inline_assembly)},
    {IR(2876, 15), static_cast<uint32_t>(kSPV_INTEL_int4)},
    {IR(2891, 19), static_cast<uint32_t>(kSPV_INTEL_io_pipes)},
    {IR(2910, 28), static_cast<uint32_t>(kSPV_INTEL_kernel_attributes)},
    {IR(2938, 26), static_cast<uint32_t>(kSPV_INTEL_long_composites)},
    {IR(2964, 20), static_cast<uint32_t>(kSPV_INTEL_loop_fuse)},
    {IR(2984, 32), static_cast<uint32_t>(kSPV_INTEL_masked_gather_scatter)},
    {IR(3016, 28), static_cast<uint32_t>(kSPV_INTEL_maximum_registers)},
    {IR(3044, 25), static_cast<uint32_t>(kSPV_INTEL_media_block_io)},
    {IR(3069, 33), static_cast<uint32_t>(kSPV_INTEL_memory_access_aliasing)},
    {IR(3102, 18), static_cast<uint32_t>(kSPV_INTEL_optnone)},
    {IR(3120, 24), static_cast<uint32_t>(kSPV_INTEL_predicated_io)},
    {IR(3144, 30), static_cast<uint32_t>(kSPV_INTEL_rounded_divide_sqrt)},
    {IR(3174, 26), static_cast<uint32_t>(kSPV_INTEL_runtime_aligned)},
    {IR(3200, 36), static_cast<uint32_t>(kSPV_INTEL_shader_integer_functions2)},
    {IR(3236, 24), static_cast<uint32_t>(kSPV_INTEL_split_barrier)},
    {IR(3260, 35), static_cast<uint32_t>(kSPV_INTEL_subgroup_buffer_prefetch)},
    {IR(3295, 46), static_cast<uint32_t>(kSPV_INTEL_subgroup_matrix_multiply_accumulate)},
    {IR(3341, 20), static_cast<uint32_t>(kSPV_INTEL_subgroups)},
    {IR(3361, 24), static_cast<uint32_t>(kSPV_INTEL_task_sequence)},
    {IR(3385, 36), static_cast<uint32_t>(kSPV_INTEL_tensor_float32_conversion)},
    {IR(3421, 35), static_cast<uint32_t>(kSPV_INTEL_ternary_bitwise_function)},
    {IR(3456, 37), static_cast<uint32_t>(kSPV_INTEL_unstructured_loop_controls)},
    {IR(3493, 30), static_cast<uint32_t>(kSPV_INTEL_usm_storage_classes)},
    {IR(3523, 32), static_cast<uint32_t>(kSPV_INTEL_variable_length_array)},
    {IR(3555, 25), static_cast<uint32_t>(kSPV_INTEL_vector_compute)},
    {IR(3580, 22), static_cast<uint32_t>(kSPV_KHR_16bit_storage)},
    {IR(3602, 21), static_cast<uint32_t>(kSPV_KHR_8bit_storage)},
    {IR(3623, 14), static_cast<uint32_t>(kSPV_KHR_abort)},
    {IR(3637, 17), static_cast<uint32_t>(kSPV_KHR_bfloat16)},
    {IR(3654, 25), static_cast<uint32_t>(kSPV_KHR_bit_instructions)},
    {IR(3679, 35), static_cast<uint32_t>(kSPV_KHR_compute_shader_derivatives)},
    {IR(3714, 22), static_cast<uint32_t>(kSPV_KHR_constant_data)},
    {IR(3736, 27), static_cast<uint32_t>(kSPV_KHR_cooperative_matrix)},
    {IR(3763, 21), static_cast<uint32_t>(kSPV_KHR_device_group)},
    {IR(3784, 22), static_cast<uint32_t>(kSPV_KHR_expect_assume)},
    {IR(3806, 23), static_cast<uint32_t>(kSPV_KHR_float_controls)},
    {IR(3829, 24), static_cast<uint32_t>(kSPV_KHR_float_controls2)},
    {IR(3853, 12), static_cast<uint32_t>(kSPV_KHR_fma)},
    {IR(3865, 36), static_cast<uint32_t>(kSPV_KHR_fragment_shader_barycentric)},
    {IR(3901, 30), static_cast<uint32_t>(kSPV_KHR_fragment_shading_rate)},
    {IR(3931, 28), static_cast<uint32_t>(kSPV_KHR_integer_dot_product)},
    {IR(3959, 21), static_cast<uint32_t>(kSPV_KHR_linkonce_odr)},
    {IR(3980, 30), static_cast<uint32_t>(kSPV_KHR_maximal_reconvergence)},
    {IR(4010, 18), static_cast<uint32_t>(kSPV_KHR_multiview)},
    {IR(4028, 35), static_cast<uint32_t>(kSPV_KHR_no_integer_wrap_decoration)},
    {IR(4063, 26), static_cast<uint32_t>(kSPV_KHR_non_semantic_info)},
    {IR(4089, 25), static_cast<uint32_t>(kSPV_KHR_opacity_micromap)},
    {IR(4114, 32), static_cast<uint32_t>(kSPV_KHR_physical_storage_buffer)},
    {IR(4146, 22), static_cast<uint32_t>(kSPV_KHR_poison_freeze)},
    {IR(4168, 28), static_cast<uint32_t>(kSPV_KHR_post_depth_coverage)},
    {IR(4196, 21), static_cast<uint32_t>(kSPV_KHR_quad_control)},
    {IR(4217, 22), static_cast<uint32_t>(kSPV_KHR_ray_cull_mask)},
    {IR(4239, 18), static_cast<uint32_t>(kSPV_KHR_ray_query)},
    {IR(4257, 20), static_cast<uint32_t>(kSPV_KHR_ray_tracing)},
    {IR(4277, 35), static_cast<uint32_t>(kSPV_KHR_ray_tracing_position_fetch)},
    {IR(4312, 37), static_cast<uint32_t>(kSPV_KHR_relaxed_extended_instruction)},
    {IR(4349, 34), static_cast<uint32_t>(kSPV_KHR_shader_atomic_counter_ops)},
    {IR(4383, 22), static_cast<uint32_t>(kSPV_KHR_shader_ballot)},
    {IR(4405, 21), static_cast<uint32_t>(kSPV_KHR_shader_clock)},
    {IR(4426, 31), static_cast<uint32_t>(kSPV_KHR_shader_draw_parameters)},
    {IR(4457, 37), static_cast<uint32_t>(kSPV_KHR_storage_buffer_storage_class)},
    {IR(4494, 24), static_cast<uint32_t>(kSPV_KHR_subgroup_rotate)},
    {IR(4518, 38), static_cast<uint32_t>(kSPV_KHR_subgroup_uniform_control_flow)},
    {IR(4556, 22), static_cast<uint32_t>(kSPV_KHR_subgroup_vote)},
    {IR(4578, 29), static_cast<uint32_t>(kSPV_KHR_terminate_invocation)},
    {IR(4607, 35), static_cast<uint32_t>(kSPV_KHR_uniform_group_instructions)},
    {IR(4642, 25), static_cast<uint32_t>(kSPV_KHR_untyped_pointers)},
    {IR(4667, 26), static_cast<uint32_t>(kSPV_KHR_variable_pointers)},
    {IR(4693, 28), static_cast<uint32_t>(kSPV_KHR_vulkan_memory_model)},
    {IR(4721, 41), static_cast<uint32_t>(kSPV_KHR_workgroup_memory_explicit_layout)},
    {IR(4762, 38), static_cast<uint32_t>(kSPV_NVX_multiview_per_view_attributes)},
    {IR(4800, 24), static_cast<uint32_t>(kSPV_NV_bindless_texture)},
    {IR(4824, 38), static_cast<uint32_t>(kSPV_NV_cluster_acceleration_structure)},
    {IR(4862, 34), static_cast<uint32_t>(kSPV_NV_compute_shader_derivatives)},
    {IR(4896, 26), static_cast<uint32_t>(kSPV_NV_cooperative_matrix)},
    {IR(4922, 27), static_cast<uint32_t>(kSPV_NV_cooperative_matrix2)},
    {IR(4949, 40), static_cast<uint32_t>(kSPV_NV_cooperative_matrix_decode_vector)},
    {IR(4989, 26), static_cast<uint32_t>(kSPV_NV_cooperative_vector)},
    {IR(5015, 29), static_cast<uint32_t>(kSPV_NV_displacement_micromap)},
    {IR(5044, 35), static_cast<uint32_t>(kSPV_NV_fragment_shader_barycentric)},
    {IR(5079, 35), static_cast<uint32_t>(kSPV_NV_geometry_shader_passthrough)},
    {IR(5114, 28), static_cast<uint32_t>(kSPV_NV_linear_swept_spheres)},
    {IR(5142, 19), static_cast<uint32_t>(kSPV_NV_mesh_shader)},
    {IR(5161, 26), static_cast<uint32_t>(kSPV_NV_push_constant_bank)},
    {IR(5187, 25), static_cast<uint32_t>(kSPV_NV_raw_access_chains)},
    {IR(5212, 19), static_cast<uint32_t>(kSPV_NV_ray_tracing)},
    {IR(5231, 31), static_cast<uint32_t>(kSPV_NV_ray_tracing_motion_blur)},
    {IR(5262, 37), static_cast<uint32_t>(kSPV_NV_sample_mask_override_coverage)},
    {IR(5299, 33), static_cast<uint32_t>(kSPV_NV_shader_atomic_fp16_vector)},
    {IR(5332, 30), static_cast<uint32_t>(kSPV_NV_shader_image_footprint)},
    {IR(5362, 33), static_cast<uint32_t>(kSPV_NV_shader_invocation_reorder)},
    {IR(5395, 26), static_cast<uint32_t>(kSPV_NV_shader_sm_builtins)},
    {IR(5421, 35), static_cast<uint32_t>(kSPV_NV_shader_subgroup_partitioned)},
    {IR(5456, 20), static_cast<uint32_t>(kSPV_NV_shading_rate)},
    {IR(5476, 29), static_cast<uint32_t>(kSPV_NV_stereo_view_rendering)},
    {IR(5505, 25), static_cast<uint32_t>(kSPV_NV_tensor_addressing)},
    {IR(5530, 23), static_cast<uint32_t>(kSPV_NV_viewport_array2)},
    {IR(5553, 39), static_cast<uint32_t>(kSPV_QCOM_cooperative_matrix_conversion)},
    {IR(5592, 26), static_cast<uint32_t>(kSPV_QCOM_image_processing)},
    {IR(5618, 27), static_cast<uint32_t>(kSPV_QCOM_image_processing2)},
    {IR(5645, 27), static_cast<uint32_t>(kSPV_QCOM_image_processing3)},
    {IR(5672, 30), static_cast<uint32_t>(kSPV_QCOM_multiple_wait_queues)},
    {IR(5702, 22), static_cast<uint32_t>(kSPV_QCOM_tile_shading)},
    {IR(5724, 22), static_cast<uint32_t>(kSPV_SAMSUNG_intrinsic)},
    {IR(5746, 38), static_cast<uint32_t>(kSPV_VALIDATOR_ignore_type_decl_unique)},
    {IR(5784, 34), static_cast<uint32_t>(kSPV_VALVE_mixed_float_dot_product)},
}};











static const std::array<OperandDesc, 1242> kOperandsByValue{{
  {0,IR(0, 0),IR(5818, 9)              ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(5834, 10)               ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(5844, 10)               ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(5854, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(5862, 11)                ,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(5883, 11)                ,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5348,IR(0, 0),IR(5894, 24)                             ,IR(0, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {0,IR(0, 0),IR(5976, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(5992, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(6002, 13)                  ,IR(0, 0),IR(4, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(6015, 13)                  ,IR(0, 0),IR(5, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(6028, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(6037, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(6048, 12)                 ,IR(0, 0),IR(6, 6),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(6138, 13)                  ,IR(0, 0),IR(12, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(6151, 6)           ,IR(0, 0),IR(14, 5),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(6197, 14)                   ,IR(0, 0),IR(19, 5),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(6245, 15)                    ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(6260, 15)                    ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(6275, 10)               ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(6285, 14)                   ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(6299, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(6309, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(0, 0),IR(6320, 12)                 ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {18,IR(0, 0),IR(6332, 9)              ,IR(0, 0),IR(25, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(6359, 15)                    ,IR(0, 0),IR(25, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {20,IR(0, 0),IR(6374, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {22,IR(0, 0),IR(6385, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {23,IR(0, 0),IR(6395, 17)                      ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {24,IR(0, 0),IR(6412, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {25,IR(0, 0),IR(6426, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {26,IR(0, 0),IR(6440, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {27,IR(0, 0),IR(6452, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {28,IR(0, 0),IR(6470, 19)                        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {29,IR(0, 0),IR(6489, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {30,IR(0, 0),IR(6510, 8)             ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {31,IR(0, 0),IR(6518, 11)                ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {32,IR(0, 0),IR(6529, 22)                           ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {33,IR(0, 0),IR(6551, 13)                  ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {34,IR(0, 0),IR(6564, 15)                    ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {36,IR(0, 0),IR(6579, 13)                  ,IR(0, 0),IR(26, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {37,IR(0, 0),IR(6626, 16)                     ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {38,IR(0, 0),IR(6642, 13)                  ,IR(0, 0),IR(29, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {39,IR(0, 0),IR(6655, 21)                          ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {40,IR(0, 0),IR(6676, 11)                ,IR(0, 0),IR(29, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {41,IR(0, 0),IR(6687, 26)                               ,IR(0, 0),IR(26, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {42,IR(0, 0),IR(6713, 12)                 ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {43,IR(0, 0),IR(6725, 14)                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4160,IR(0, 0),IR(6739, 10)               ,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4161,IR(0, 0),IR(6765, 13)                  ,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4162,IR(0, 0),IR(6778, 13)                  ,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4163,IR(0, 0),IR(6791, 10)               ,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4164,IR(0, 0),IR(6801, 13)                  ,IR(0, 0),IR(31, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4416,IR(0, 0),IR(6814, 15)                    ,IR(1, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4417,IR(0, 0),IR(6869, 15)                    ,IR(2, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4418,IR(0, 0),IR(6902, 15)                    ,IR(3, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4419,IR(0, 0),IR(6935, 15)                    ,IR(4, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4420,IR(0, 0),IR(6968, 15)                    ,IR(5, 1),IR(32, 2),IR(2, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4424,IR(0, 0),IR(7001, 11)                ,IR(0, 0),IR(34, 1),IR(3, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4425,IR(0, 0),IR(7027, 13)                  ,IR(0, 0),IR(34, 1),IR(3, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4426,IR(0, 0),IR(7040, 10)               ,IR(0, 0),IR(35, 3),IR(4, 3),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4432,IR(0, 0),IR(7050, 24)                             ,IR(0, 0),IR(38, 1),IR(7, 1),0xffffffffu,0xffffffffu},
  {4438,IR(0, 0),IR(7097, 12)                 ,IR(0, 0),IR(39, 1),IR(8, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4440,IR(0, 0),IR(7121, 10)               ,IR(0, 0),IR(40, 1),IR(9, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4444,IR(0, 0),IR(7141, 15)                    ,IR(0, 0),IR(38, 1),IR(7, 1),0xffffffffu,0xffffffffu},
  {4492,IR(0, 0),IR(7156, 15)                    ,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4493,IR(0, 0),IR(7187, 18)                       ,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4494,IR(0, 0),IR(7205, 18)                       ,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4992,IR(0, 0),IR(7223, 20)                         ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {4993,IR(0, 0),IR(7243, 28)                                 ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {4994,IR(0, 0),IR(7271, 26)                               ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {4995,IR(0, 0),IR(7297, 19)                        ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {4996,IR(0, 0),IR(7316, 27)                                ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {4997,IR(0, 0),IR(7343, 25)                              ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {4998,IR(0, 0),IR(7368, 22)                           ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {5014,IR(0, 0),IR(7390, 18)                       ,IR(0, 0),IR(42, 1),IR(11, 1),0xffffffffu,0xffffffffu},
  {5021,IR(0, 0),IR(7425, 29)                                  ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5073,IR(0, 0),IR(7472, 16)                     ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5122,IR(0, 0),IR(7488, 15)                    ,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5123,IR(0, 0),IR(7521, 16)                     ,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5253,IR(0, 0),IR(7537, 15)                    ,IR(0, 0),IR(45, 2),IR(12, 2),0xffffffffu,0xffffffffu},
  {5257,IR(0, 0),IR(7573, 20)                         ,IR(0, 0),IR(47, 1),IR(14, 1),0xffffffffu,0xffffffffu},
  {5258,IR(0, 0),IR(7612, 24)                             ,IR(0, 0),IR(47, 1),IR(14, 1),0xffffffffu,0xffffffffu},
  {5261,IR(0, 0),IR(7636, 18)                       ,IR(0, 0),IR(48, 2),IR(15, 2),0xffffffffu,0xffffffffu},
  {5262,IR(0, 0),IR(7674, 22)                           ,IR(0, 0),IR(48, 2),IR(15, 2),0xffffffffu,0xffffffffu},
  {5264,IR(0, 0),IR(7696, 16)                     ,IR(0, 0),IR(50, 1),IR(17, 1),0xffffffffu,0xffffffffu},
  {5274,IR(0, 0),IR(7736, 12)                 ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5275,IR(0, 0),IR(7748, 17)                      ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5276,IR(0, 0),IR(7765, 19)                        ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5277,IR(0, 0),IR(7784, 22)                           ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5278,IR(0, 0),IR(7806, 22)                           ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5279,IR(0, 0),IR(7828, 15)                    ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5280,IR(0, 0),IR(7843, 16)                     ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5281,IR(0, 0),IR(7859, 18)                       ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5286,IR(0, 0),IR(7877, 13)                  ,IR(6, 1),IR(52, 1),IR(19, 2),0xffffffffu,0xffffffffu},
  {5287,IR(0, 0),IR(7925, 20)                         ,IR(7, 1),IR(52, 1),IR(19, 2),0xffffffffu,0xffffffffu},
  {5292,IR(0, 0),IR(7964, 12)                 ,IR(8, 1),IR(53, 1),IR(21, 2),0xffffffffu,0xffffffffu},
  {5293,IR(0, 0),IR(8010, 23)                            ,IR(9, 1),IR(53, 1),IR(21, 2),0xffffffffu,0xffffffffu},
  {5294,IR(0, 0),IR(8055, 25)                              ,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu},
  {5295,IR(0, 0),IR(8080, 24)                             ,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu},
  {5296,IR(0, 0),IR(8104, 28)                                 ,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu},
  {5299,IR(0, 0),IR(8132, 17)                      ,IR(0, 0),IR(54, 1),IR(23, 1),0xffffffffu,0xffffffffu},
  {5319,IR(0, 0),IR(8149, 12)                 ,IR(10, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5320,IR(0, 0),IR(8172, 14)                   ,IR(11, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5321,IR(0, 0),IR(8199, 18)                       ,IR(12, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5322,IR(0, 0),IR(8234, 21)                          ,IR(13, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5323,IR(0, 0),IR(8275, 19)                        ,IR(14, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5324,IR(0, 0),IR(8312, 22)                           ,IR(15, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5325,IR(0, 0),IR(8355, 11)                ,IR(16, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5326,IR(0, 0),IR(8376, 11)                ,IR(17, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5327,IR(0, 0),IR(8397, 23)                            ,IR(18, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5330,IR(0, 0),IR(8442, 17)                      ,IR(19, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5331,IR(0, 0),IR(8475, 17)                      ,IR(20, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5332,IR(0, 0),IR(8508, 7)            ,IR(0, 0),IR(57, 1),IR(26, 1),0xffffffffu,0xffffffffu},
  {5333,IR(0, 0),IR(8515, 11)                ,IR(21, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5334,IR(0, 0),IR(8536, 17)                      ,IR(0, 0),IR(58, 1),IR(27, 1),0xffffffffu,0xffffffffu},
  {5335,IR(0, 0),IR(8576, 30)                                   ,IR(0, 0),IR(59, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5337,IR(0, 0),IR(8633, 34)                                       ,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5344,IR(0, 0),IR(8700, 37)                                          ,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5351,IR(0, 0),IR(8737, 20)                         ,IR(22, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5352,IR(0, 0),IR(8776, 20)                         ,IR(0, 0),IR(61, 1),IR(28, 1),0xffffffffu,0xffffffffu},
  {5359,IR(0, 0),IR(8796, 14)                   ,IR(0, 0),IR(62, 1),IR(29, 1),0xffffffffu,0xffffffffu},
  {5360,IR(0, 0),IR(8838, 11)                ,IR(0, 0),IR(63, 1),IR(29, 1),0xffffffffu,0xffffffffu},
  {5361,IR(0, 0),IR(8888, 20)                         ,IR(0, 0),IR(62, 1),IR(29, 1),0xffffffffu,0xffffffffu},
  {5374,IR(0, 0),IR(8908, 13)                  ,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu},
  {5375,IR(0, 0),IR(8940, 10)               ,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu},
  {5376,IR(0, 0),IR(8950, 9)              ,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu},
  {5377,IR(0, 0),IR(8959, 7)            ,IR(0, 0),IR(64, 1),IR(30, 1),0xffffffffu,0xffffffffu},
  {5396,IR(0, 0),IR(8966, 18)                       ,IR(0, 0),IR(63, 1),IR(29, 1),0xffffffffu,0xffffffffu},
  {5405,IR(0, 0),IR(8984, 34)                                       ,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5406,IR(0, 0),IR(9018, 33)                                      ,IR(0, 0),IR(60, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5420,IR(0, 0),IR(9051, 18)                       ,IR(0, 0),IR(62, 1),IR(29, 1),0xffffffffu,0xffffffffu},
  {5421,IR(0, 0),IR(9069, 14)                   ,IR(0, 0),IR(63, 1),IR(29, 1),0xffffffffu,0xffffffffu},
  {5436,IR(0, 0),IR(9083, 12)                 ,IR(0, 0),IR(65, 1),IR(31, 1),0xffffffffu,0xffffffffu},
  {6021,IR(0, 0),IR(9136, 12)                 ,IR(0, 0),IR(66, 1),IR(32, 1),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(9163, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(5985, 7)            ,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(6060, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(6069, 13)                  ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(5873, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(9170, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(5827, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(9178, 9)              ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(9187, 14)                   ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(9201, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(9209, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(9217, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(9223, 13)                  ,IR(0, 0),IR(68, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(9236, 11)                ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(9247, 15)                    ,IR(0, 0),IR(69, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(9262, 12)                 ,IR(0, 0),IR(69, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(0, 0),IR(9274, 6)           ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {18,IR(0, 0),IR(9280, 7)            ,IR(0, 0),IR(1, 0),IR(33, 1),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(9287, 14)                   ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {20,IR(0, 0),IR(9301, 15)                    ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {21,IR(0, 0),IR(9316, 14)                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {22,IR(0, 0),IR(9330, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {23,IR(0, 0),IR(9336, 22)                           ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {24,IR(0, 0),IR(9358, 18)                       ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {25,IR(0, 0),IR(9376, 20)                         ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {27,IR(0, 0),IR(9396, 24)                             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {28,IR(0, 0),IR(9420, 34)                                       ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {29,IR(0, 0),IR(9454, 33)                                      ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {30,IR(0, 0),IR(9487, 34)                                       ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {31,IR(0, 0),IR(9521, 33)                                      ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {32,IR(0, 0),IR(6002, 13)                  ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {33,IR(0, 0),IR(6015, 13)                  ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {34,IR(0, 0),IR(9554, 15)                    ,IR(0, 0),IR(71, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {35,IR(0, 0),IR(6341, 18)                       ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {36,IR(0, 0),IR(9586, 10)               ,IR(0, 0),IR(72, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {37,IR(0, 0),IR(9596, 12)                 ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {38,IR(0, 0),IR(9608, 15)                    ,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {39,IR(0, 0),IR(9623, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {40,IR(0, 0),IR(9628, 16)                     ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {41,IR(0, 0),IR(9644, 16)                     ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {42,IR(0, 0),IR(9660, 7)            ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {43,IR(0, 0),IR(9667, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {44,IR(0, 0),IR(9677, 8)             ,IR(0, 0),IR(73, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {45,IR(0, 0),IR(9569, 17)                      ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {46,IR(0, 0),IR(9685, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {47,IR(0, 0),IR(9699, 12)                 ,IR(0, 0),IR(74, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {48,IR(0, 0),IR(9711, 13)                  ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {49,IR(0, 0),IR(9724, 28)                                 ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {50,IR(0, 0),IR(9752, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {51,IR(0, 0),IR(9763, 18)                       ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {52,IR(0, 0),IR(9781, 22)                           ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {53,IR(0, 0),IR(9803, 18)                       ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {54,IR(0, 0),IR(9821, 16)                     ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {55,IR(0, 0),IR(9837, 30)                                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {56,IR(0, 0),IR(9867, 31)                                    ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {57,IR(0, 0),IR(6211, 14)                   ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {58,IR(0, 0),IR(9898, 17)                      ,IR(0, 0),IR(75, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {59,IR(0, 0),IR(9915, 13)                  ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {60,IR(0, 0),IR(9928, 12)                 ,IR(0, 0),IR(76, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {61,IR(0, 0),IR(6592, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {62,IR(0, 0),IR(9940, 20)                         ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {63,IR(0, 0),IR(9960, 26)                               ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {64,IR(0, 0),IR(6847, 22)                           ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {65,IR(0, 0),IR(9986, 23)                            ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {66,IR(0, 0),IR(10009, 31)                                    ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {67,IR(0, 0),IR(10040, 25)                              ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {68,IR(0, 0),IR(10065, 20)                         ,IR(0, 0),IR(77, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {69,IR(0, 0),IR(6157, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {70,IR(0, 0),IR(6225, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {71,IR(0, 0),IR(10085, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {4165,IR(0, 0),IR(6749, 16)                     ,IR(0, 0),IR(1, 0),IR(34, 1),0xffffffffu,0xffffffffu},
  {4166,IR(0, 0),IR(10103, 28)                                 ,IR(0, 0),IR(1, 0),IR(35, 1),0xffffffffu,0xffffffffu},
  {4167,IR(0, 0),IR(10131, 28)                                 ,IR(0, 0),IR(1, 0),IR(35, 1),0xffffffffu,0xffffffffu},
  {4168,IR(0, 0),IR(10159, 30)                                   ,IR(0, 0),IR(1, 0),IR(35, 1),0xffffffffu,0xffffffffu},
  {4174,IR(0, 0),IR(10189, 11)                ,IR(0, 0),IR(1, 0),IR(36, 1),0xffffffffu,0xffffffffu},
  {4175,IR(0, 0),IR(10200, 37)                                          ,IR(0, 0),IR(1, 0),IR(36, 1),0xffffffffu,0xffffffffu},
  {4176,IR(0, 0),IR(10237, 40)                                             ,IR(0, 0),IR(1, 0),IR(36, 1),0xffffffffu,0xffffffffu},
  {4191,IR(0, 0),IR(10277, 9)              ,IR(0, 0),IR(1, 0),IR(37, 1),0xffffffffu,0xffffffffu},
  {4201,IR(0, 0),IR(10286, 28)                                 ,IR(0, 0),IR(1, 0),IR(38, 1),0xffffffffu,0xffffffffu},
  {4212,IR(0, 0),IR(10314, 10)               ,IR(0, 0),IR(1, 0),IR(39, 1),0xffffffffu,0xffffffffu},
  {4213,IR(0, 0),IR(10324, 27)                                ,IR(0, 0),IR(78, 2),IR(39, 1),0xffffffffu,0xffffffffu},
  {4228,IR(0, 0),IR(10372, 10)               ,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu},
  {4229,IR(0, 0),IR(10382, 10)               ,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu},
  {4230,IR(0, 0),IR(10392, 22)                           ,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu},
  {4231,IR(0, 0),IR(10414, 10)               ,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu},
  {4232,IR(0, 0),IR(10424, 18)                       ,IR(0, 0),IR(1, 0),IR(40, 1),0xffffffffu,0xffffffffu},
  {4422,IR(0, 0),IR(7074, 23)                            ,IR(0, 0),IR(3, 1),IR(7, 1),0xffffffffu,0xffffffffu},
  {4423,IR(0, 0),IR(6608, 18)                       ,IR(0, 0),IR(1, 0),IR(2, 1),0xffffffffu,0xffffffffu},
  {4427,IR(0, 0),IR(7012, 15)                    ,IR(0, 0),IR(3, 1),IR(3, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4428,IR(0, 0),IR(10442, 33)                                      ,IR(0, 0),IR(3, 1),IR(41, 1),0xffffffffu,0xffffffffu},
  {4429,IR(0, 0),IR(10475, 43)                                                ,IR(0, 0),IR(80, 1),IR(41, 1),0xffffffffu,0xffffffffu},
  {4430,IR(0, 0),IR(10518, 44)                                                 ,IR(0, 0),IR(80, 1),IR(41, 1),0xffffffffu,0xffffffffu},
  {4431,IR(0, 0),IR(10562, 16)                     ,IR(0, 0),IR(1, 0),IR(42, 1),0xffffffffu,0xffffffffu},
  {4433,IR(0, 0),IR(10578, 25)                              ,IR(23, 1),IR(1, 0),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4434,IR(0, 0),IR(10631, 35)                                        ,IR(24, 1),IR(81, 1),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4435,IR(0, 0),IR(10683, 22)                           ,IR(0, 0),IR(1, 0),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4436,IR(0, 0),IR(10705, 21)                          ,IR(0, 0),IR(1, 0),IR(43, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4437,IR(0, 0),IR(7109, 12)                 ,IR(0, 0),IR(1, 0),IR(8, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4439,IR(0, 0),IR(7131, 10)               ,IR(0, 0),IR(3, 1),IR(9, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4441,IR(0, 0),IR(10726, 30)                                   ,IR(0, 0),IR(3, 1),IR(44, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4442,IR(0, 0),IR(10756, 17)                      ,IR(0, 0),IR(82, 1),IR(44, 1),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4445,IR(0, 0),IR(10773, 17)                      ,IR(0, 0),IR(83, 1),IR(45, 1),0xffffffffu,0xffffffffu},
  {4447,IR(0, 0),IR(10790, 28)                                 ,IR(0, 0),IR(1, 0),IR(46, 1),0xffffffffu,0xffffffffu},
  {4448,IR(0, 0),IR(10818, 24)                             ,IR(0, 0),IR(1, 0),IR(47, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {4449,IR(0, 0),IR(10842, 34)                                       ,IR(0, 0),IR(84, 1),IR(47, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {4450,IR(0, 0),IR(10876, 21)                          ,IR(0, 0),IR(1, 0),IR(47, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {4464,IR(0, 0),IR(10897, 15)                    ,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4465,IR(0, 0),IR(10912, 18)                       ,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4466,IR(0, 0),IR(10930, 25)                              ,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4467,IR(0, 0),IR(10955, 16)                     ,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4468,IR(0, 0),IR(10971, 16)                     ,IR(0, 0),IR(1, 0),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4471,IR(0, 0),IR(10987, 23)                            ,IR(0, 0),IR(3, 1),IR(49, 1),0xffffffffu,0xffffffffu},
  {4472,IR(0, 0),IR(11010, 12)                 ,IR(0, 0),IR(3, 1),IR(49, 1),0xffffffffu,0xffffffffu},
  {4473,IR(0, 0),IR(11022, 19)                        ,IR(0, 0),IR(1, 0),IR(50, 1),0xffffffffu,0xffffffffu},
  {4478,IR(0, 0),IR(11041, 32)                                     ,IR(0, 0),IR(85, 2),IR(51, 2),0xffffffffu,0xffffffffu},
  {4479,IR(0, 0),IR(6095, 14)                   ,IR(0, 0),IR(3, 1),IR(28, 1),0xffffffffu,0xffffffffu},
  {4484,IR(0, 0),IR(11073, 26)                               ,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu},
  {4485,IR(0, 0),IR(11099, 21)                          ,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu},
  {4486,IR(0, 0),IR(11120, 22)                           ,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu},
  {4495,IR(0, 0),IR(7171, 16)                     ,IR(0, 0),IR(3, 1),IR(54, 1),0xffffffffu,0xffffffffu},
  {4496,IR(0, 0),IR(11142, 32)                                     ,IR(0, 0),IR(87, 1),IR(55, 1),0xffffffffu,0xffffffffu},
  {4498,IR(0, 0),IR(11174, 23)                            ,IR(0, 0),IR(1, 0),IR(56, 1),0xffffffffu,0xffffffffu},
  {4539,IR(0, 0),IR(11197, 23)                            ,IR(0, 0),IR(1, 0),IR(57, 1),0xffffffffu,0xffffffffu},
  {4543,IR(0, 0),IR(11220, 22)                           ,IR(0, 0),IR(1, 0),IR(58, 1),0xffffffffu,0xffffffffu},
  {4544,IR(0, 0),IR(11242, 29)                                  ,IR(0, 0),IR(1, 0),IR(58, 1),0xffffffffu,0xffffffffu},
  {5008,IR(0, 0),IR(11271, 16)                     ,IR(0, 0),IR(3, 1),IR(59, 1),0xffffffffu,0xffffffffu},
  {5009,IR(0, 0),IR(11287, 22)                           ,IR(0, 0),IR(3, 1),IR(60, 1),0xffffffffu,0xffffffffu},
  {5010,IR(0, 0),IR(11309, 16)                     ,IR(0, 0),IR(3, 1),IR(61, 1),0xffffffffu,0xffffffffu},
  {5013,IR(0, 0),IR(7408, 17)                      ,IR(0, 0),IR(3, 1),IR(11, 1),0xffffffffu,0xffffffffu},
  {5015,IR(0, 0),IR(11325, 21)                          ,IR(0, 0),IR(3, 1),IR(62, 1),0xffffffffu,0xffffffffu},
  {5016,IR(0, 0),IR(11346, 14)                   ,IR(0, 0),IR(68, 1),IR(63, 1),0xffffffffu,0xffffffffu},
  {5055,IR(0, 0),IR(11360, 15)                    ,IR(0, 0),IR(1, 0),IR(64, 1),0xffffffffu,0xffffffffu},
  {5067,IR(0, 0),IR(7454, 18)                       ,IR(0, 0),IR(3, 1),IR(65, 1),0xffffffffu,0xffffffffu},
  {5087,IR(0, 0),IR(11375, 15)                    ,IR(0, 0),IR(1, 0),IR(66, 1),0xffffffffu,0xffffffffu},
  {5112,IR(0, 0),IR(11390, 14)                   ,IR(0, 0),IR(1, 0),IR(67, 1),0xffffffffu,0xffffffffu},
  {5114,IR(0, 0),IR(11404, 27)                                ,IR(0, 0),IR(88, 2),IR(67, 1),0xffffffffu,0xffffffffu},
  {5116,IR(0, 0),IR(11431, 16)                     ,IR(0, 0),IR(1, 0),IR(68, 1),0xffffffffu,0xffffffffu},
  {5117,IR(0, 0),IR(11447, 22)                           ,IR(0, 0),IR(90, 1),IR(68, 1),0xffffffffu,0xffffffffu},
  {5118,IR(0, 0),IR(11469, 29)                                  ,IR(0, 0),IR(91, 2),IR(68, 1),0xffffffffu,0xffffffffu},
  {5120,IR(0, 0),IR(11498, 9)              ,IR(0, 0),IR(1, 0),IR(69, 1),0xffffffffu,0xffffffffu},
  {5128,IR(0, 0),IR(7503, 18)                       ,IR(0, 0),IR(93, 1),IR(70, 1),0xffffffffu,0xffffffffu},
  {5146,IR(0, 0),IR(11507, 16)                     ,IR(0, 0),IR(1, 0),IR(71, 1),0xffffffffu,0xffffffffu},
  {5156,IR(0, 0),IR(11523, 16)                     ,IR(0, 0),IR(1, 0),IR(72, 1),0xffffffffu,0xffffffffu},
  {5181,IR(0, 0),IR(11539, 15)                    ,IR(0, 0),IR(94, 1),IR(73, 1),0xffffffffu,0xffffffffu},
  {5249,IR(0, 0),IR(11554, 29)                                  ,IR(0, 0),IR(25, 1),IR(74, 1),0xffffffffu,0xffffffffu},
  {5251,IR(0, 0),IR(11583, 28)                                 ,IR(0, 0),IR(70, 1),IR(75, 1),0xffffffffu,0xffffffffu},
  {5254,IR(0, 0),IR(6169, 28)                                 ,IR(25, 1),IR(95, 1),IR(76, 2),0xffffffffu,0xffffffffu},
  {5255,IR(0, 0),IR(7552, 21)                          ,IR(0, 0),IR(96, 1),IR(78, 1),0xffffffffu,0xffffffffu},
  {5259,IR(0, 0),IR(7593, 19)                        ,IR(0, 0),IR(97, 1),IR(14, 1),0xffffffffu,0xffffffffu},
  {5260,IR(0, 0),IR(7654, 20)                         ,IR(0, 0),IR(40, 1),IR(79, 1),0xffffffffu,0xffffffffu},
  {5265,IR(0, 0),IR(7712, 24)                             ,IR(0, 0),IR(3, 1),IR(17, 1),0xffffffffu,0xffffffffu},
  {5266,IR(0, 0),IR(6109, 14)                   ,IR(0, 0),IR(3, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5282,IR(0, 0),IR(11638, 17)                      ,IR(0, 0),IR(1, 0),IR(80, 1),0xffffffffu,0xffffffffu},
  {5283,IR(0, 0),IR(6123, 15)                    ,IR(0, 0),IR(3, 1),IR(23, 1),0xffffffffu,0xffffffffu},
  {5284,IR(0, 0),IR(7902, 23)                            ,IR(26, 1),IR(1, 0),IR(19, 2),0xffffffffu,0xffffffffu},
  {5288,IR(0, 0),IR(11677, 31)                                    ,IR(27, 1),IR(3, 1),IR(81, 2),0xffffffffu,0xffffffffu},
  {5291,IR(0, 0),IR(7991, 19)                        ,IR(28, 1),IR(3, 1),IR(21, 2),0xffffffffu,0xffffffffu},
  {5297,IR(0, 0),IR(11752, 30)                                   ,IR(29, 1),IR(1, 0),IR(83, 2),0xffffffffu,0xffffffffu},
  {5301,IR(0, 0),IR(11811, 17)                      ,IR(30, 1),IR(3, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5302,IR(0, 0),IR(11848, 23)                            ,IR(31, 1),IR(3, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5303,IR(0, 0),IR(11897, 36)                                         ,IR(32, 1),IR(98, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5304,IR(0, 0),IR(11972, 39)                                            ,IR(33, 1),IR(74, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5305,IR(0, 0),IR(12053, 39)                                            ,IR(34, 1),IR(99, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5306,IR(0, 0),IR(12134, 37)                                          ,IR(35, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5307,IR(0, 0),IR(12211, 36)                                         ,IR(36, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5308,IR(0, 0),IR(12286, 37)                                          ,IR(37, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5309,IR(0, 0),IR(12363, 36)                                         ,IR(38, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5310,IR(0, 0),IR(12438, 39)                                            ,IR(39, 1),IR(101, 2),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5311,IR(0, 0),IR(12519, 42)                                               ,IR(40, 1),IR(103, 2),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5312,IR(0, 0),IR(12606, 42)                                               ,IR(41, 1),IR(105, 2),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5336,IR(0, 0),IR(8606, 27)                                ,IR(0, 0),IR(3, 1),IR(86, 1),0xffffffffu,0xffffffffu},
  {5340,IR(0, 0),IR(6082, 13)                  ,IR(0, 0),IR(3, 1),IR(26, 1),0xffffffffu,0xffffffffu},
  {5341,IR(0, 0),IR(8553, 23)                            ,IR(0, 0),IR(3, 1),IR(27, 1),0xffffffffu,0xffffffffu},
  {5345,IR(0, 0),IR(12693, 18)                       ,IR(42, 1),IR(1, 0),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5346,IR(0, 0),IR(12732, 29)                                  ,IR(43, 1),IR(1, 0),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5347,IR(0, 0),IR(5945, 31)                                    ,IR(44, 1),IR(3, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5350,IR(0, 0),IR(12827, 32)                                     ,IR(45, 1),IR(3, 1),IR(81, 2),0xffffffffu,0xffffffffu},
  {5353,IR(0, 0),IR(12890, 25)                              ,IR(0, 0),IR(3, 1),IR(28, 1),0xffffffffu,0xffffffffu},
  {5357,IR(0, 0),IR(12915, 20)                         ,IR(0, 0),IR(3, 1),IR(88, 1),0xffffffffu,0xffffffffu},
  {5363,IR(0, 0),IR(12935, 33)                                      ,IR(0, 0),IR(3, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5372,IR(0, 0),IR(12968, 38)                                           ,IR(0, 0),IR(3, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5373,IR(0, 0),IR(8921, 19)                        ,IR(0, 0),IR(3, 1),IR(30, 1),0xffffffffu,0xffffffffu},
  {5378,IR(0, 0),IR(13006, 32)                                     ,IR(0, 0),IR(3, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5379,IR(0, 0),IR(13038, 25)                              ,IR(46, 1),IR(3, 1),IR(90, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {5380,IR(0, 0),IR(13091, 23)                            ,IR(0, 0),IR(3, 1),IR(91, 1),0xffffffffu,0xffffffffu},
  {5381,IR(0, 0),IR(13114, 29)                                  ,IR(47, 1),IR(3, 1),IR(92, 2),0xffffffffu,0xffffffffu},
  {5383,IR(0, 0),IR(13172, 26)                               ,IR(0, 0),IR(61, 1),IR(94, 1),0xffffffffu,0xffffffffu},
  {5388,IR(0, 0),IR(13198, 27)                                ,IR(0, 0),IR(61, 1),IR(95, 1),0xffffffffu,0xffffffffu},
  {5390,IR(0, 0),IR(13225, 18)                       ,IR(0, 0),IR(1, 0),IR(96, 1),0xffffffffu,0xffffffffu},
  {5391,IR(0, 0),IR(13243, 25)                              ,IR(0, 0),IR(3, 1),IR(86, 1),0xffffffffu,0xffffffffu},
  {5394,IR(0, 0),IR(13268, 20)                         ,IR(0, 0),IR(1, 0),IR(97, 1),0xffffffffu,0xffffffffu},
  {5404,IR(0, 0),IR(13288, 22)                           ,IR(0, 0),IR(1, 0),IR(98, 1),0xffffffffu,0xffffffffu},
  {5409,IR(0, 0),IR(8667, 33)                                      ,IR(0, 0),IR(61, 1),IR(91, 1),0xffffffffu,0xffffffffu},
  {5414,IR(0, 0),IR(13310, 18)                       ,IR(0, 0),IR(1, 0),IR(99, 1),0xffffffffu,0xffffffffu},
  {5418,IR(0, 0),IR(8810, 28)                                 ,IR(0, 0),IR(1, 0),IR(29, 1),0xffffffffu,0xffffffffu},
  {5419,IR(0, 0),IR(8849, 39)                                            ,IR(0, 0),IR(1, 0),IR(29, 1),0xffffffffu,0xffffffffu},
  {5423,IR(0, 0),IR(13328, 20)                         ,IR(0, 0),IR(3, 1),IR(100, 1),0xffffffffu,0xffffffffu},
  {5425,IR(0, 0),IR(13348, 14)                   ,IR(0, 0),IR(1, 0),IR(101, 1),0xffffffffu,0xffffffffu},
  {5426,IR(0, 0),IR(13362, 23)                            ,IR(0, 0),IR(1, 0),IR(102, 1),0xffffffffu,0xffffffffu},
  {5430,IR(0, 0),IR(13385, 30)                                   ,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu},
  {5431,IR(0, 0),IR(13415, 31)                                    ,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu},
  {5432,IR(0, 0),IR(13446, 40)                                             ,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu},
  {5433,IR(0, 0),IR(13486, 36)                                         ,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu},
  {5434,IR(0, 0),IR(13522, 30)                                   ,IR(0, 0),IR(1, 0),IR(103, 1),0xffffffffu,0xffffffffu},
  {5435,IR(0, 0),IR(13552, 28)                                 ,IR(0, 0),IR(1, 0),IR(97, 1),0xffffffffu,0xffffffffu},
  {5437,IR(0, 0),IR(9095, 41)                                              ,IR(0, 0),IR(61, 1),IR(31, 1),0xffffffffu,0xffffffffu},
  {5439,IR(0, 0),IR(13580, 19)                        ,IR(0, 0),IR(1, 0),IR(104, 1),0xffffffffu,0xffffffffu},
  {5447,IR(0, 0),IR(13599, 32)                                     ,IR(0, 0),IR(107, 1),IR(105, 1),0xffffffffu,0xffffffffu},
  {5568,IR(0, 0),IR(13631, 21)                          ,IR(0, 0),IR(1, 0),IR(106, 1),0xffffffffu,0xffffffffu},
  {5569,IR(0, 0),IR(13652, 27)                                ,IR(0, 0),IR(1, 0),IR(106, 1),0xffffffffu,0xffffffffu},
  {5570,IR(0, 0),IR(13679, 26)                               ,IR(0, 0),IR(1, 0),IR(106, 1),0xffffffffu,0xffffffffu},
  {5579,IR(0, 0),IR(13705, 31)                                    ,IR(0, 0),IR(1, 0),IR(107, 1),0xffffffffu,0xffffffffu},
  {5582,IR(0, 0),IR(13736, 21)                          ,IR(0, 0),IR(1, 0),IR(108, 1),0xffffffffu,0xffffffffu},
  {5583,IR(0, 0),IR(13757, 23)                            ,IR(0, 0),IR(1, 0),IR(108, 1),0xffffffffu,0xffffffffu},
  {5584,IR(0, 0),IR(13780, 23)                            ,IR(0, 0),IR(1, 0),IR(109, 1),0xffffffffu,0xffffffffu},
  {5603,IR(0, 0),IR(13803, 22)                           ,IR(0, 0),IR(1, 0),IR(110, 1),0xffffffffu,0xffffffffu},
  {5604,IR(0, 0),IR(13825, 24)                             ,IR(0, 0),IR(1, 0),IR(110, 1),0xffffffffu,0xffffffffu},
  {5606,IR(0, 0),IR(13849, 9)              ,IR(0, 0),IR(1, 0),IR(111, 1),0xffffffffu,0xffffffffu},
  {5612,IR(0, 0),IR(13858, 23)                            ,IR(0, 0),IR(1, 0),IR(112, 1),0xffffffffu,0xffffffffu},
  {5613,IR(0, 0),IR(13881, 23)                            ,IR(0, 0),IR(1, 0),IR(112, 1),0xffffffffu,0xffffffffu},
  {5616,IR(0, 0),IR(13904, 23)                            ,IR(0, 0),IR(1, 0),IR(112, 1),0xffffffffu,0xffffffffu},
  {5617,IR(0, 0),IR(13927, 19)                        ,IR(0, 0),IR(108, 1),IR(113, 1),0xffffffffu,0xffffffffu},
  {5619,IR(0, 0),IR(13946, 15)                    ,IR(0, 0),IR(1, 0),IR(113, 1),0xffffffffu,0xffffffffu},
  {5629,IR(0, 0),IR(13961, 16)                     ,IR(0, 0),IR(1, 0),IR(114, 1),0xffffffffu,0xffffffffu},
  {5696,IR(0, 0),IR(13977, 33)                                      ,IR(0, 0),IR(1, 0),IR(115, 1),0xffffffffu,0xffffffffu},
  {5697,IR(0, 0),IR(14010, 38)                                           ,IR(0, 0),IR(1, 0),IR(115, 1),0xffffffffu,0xffffffffu},
  {5698,IR(0, 0),IR(14048, 39)                                            ,IR(0, 0),IR(1, 0),IR(115, 1),0xffffffffu,0xffffffffu},
  {5817,IR(0, 0),IR(14087, 25)                              ,IR(0, 0),IR(1, 0),IR(116, 1),0xffffffffu,0xffffffffu},
  {5821,IR(0, 0),IR(14112, 26)                               ,IR(0, 0),IR(1, 0),IR(108, 1),0xffffffffu,0xffffffffu},
  {5824,IR(0, 0),IR(14138, 27)                                ,IR(48, 1),IR(1, 0),IR(117, 2),0xffffffffu,0xffffffffu},
  {5837,IR(0, 0),IR(14191, 20)                         ,IR(0, 0),IR(0, 1),IR(119, 1),0xffffffffu,0xffffffffu},
  {5844,IR(0, 0),IR(14211, 33)                                      ,IR(49, 1),IR(1, 0),IR(120, 2),0xffffffffu,0xffffffffu},
  {5845,IR(0, 0),IR(14276, 38)                                           ,IR(50, 1),IR(1, 0),IR(122, 2),0xffffffffu,0xffffffffu},
  {5886,IR(0, 0),IR(14351, 30)                                   ,IR(0, 0),IR(1, 0),IR(124, 1),0xffffffffu,0xffffffffu},
  {5888,IR(0, 0),IR(14381, 23)                            ,IR(51, 1),IR(1, 0),IR(125, 2),0xffffffffu,0xffffffffu},
  {5892,IR(0, 0),IR(14426, 22)                           ,IR(0, 0),IR(1, 0),IR(127, 1),0xffffffffu,0xffffffffu},
  {5897,IR(0, 0),IR(14448, 26)                               ,IR(0, 0),IR(1, 0),IR(127, 1),0xffffffffu,0xffffffffu},
  {5898,IR(0, 0),IR(14474, 25)                              ,IR(52, 1),IR(1, 0),IR(128, 2),0xffffffffu,0xffffffffu},
  {5904,IR(0, 0),IR(14523, 28)                                 ,IR(53, 1),IR(1, 0),IR(130, 2),0xffffffffu,0xffffffffu},
  {5906,IR(0, 0),IR(14578, 15)                    ,IR(54, 1),IR(1, 0),IR(132, 2),0xffffffffu,0xffffffffu},
  {5908,IR(0, 0),IR(14607, 21)                          ,IR(55, 1),IR(1, 0),IR(134, 2),0xffffffffu,0xffffffffu},
  {5910,IR(0, 0),IR(14648, 26)                               ,IR(0, 0),IR(1, 0),IR(136, 1),0xffffffffu,0xffffffffu},
  {5916,IR(0, 0),IR(14674, 41)                                              ,IR(56, 1),IR(1, 0),IR(137, 2),0xffffffffu,0xffffffffu},
  {5920,IR(0, 0),IR(14755, 25)                              ,IR(57, 1),IR(1, 0),IR(139, 2),0xffffffffu,0xffffffffu},
  {5922,IR(0, 0),IR(14804, 35)                                        ,IR(58, 1),IR(1, 0),IR(141, 2),0xffffffffu,0xffffffffu},
  {5935,IR(0, 0),IR(14873, 24)                             ,IR(59, 1),IR(1, 0),IR(143, 2),0xffffffffu,0xffffffffu},
  {5939,IR(0, 0),IR(14920, 30)                                   ,IR(60, 1),IR(1, 0),IR(145, 2),0xffffffffu,0xffffffffu},
  {5943,IR(0, 0),IR(14979, 14)                   ,IR(61, 1),IR(1, 0),IR(147, 2),0xffffffffu,0xffffffffu},
  {5945,IR(0, 0),IR(15006, 20)                         ,IR(62, 1),IR(1, 0),IR(149, 2),0xffffffffu,0xffffffffu},
  {5948,IR(0, 0),IR(15045, 14)                   ,IR(63, 1),IR(1, 0),IR(151, 2),0xffffffffu,0xffffffffu},
  {6016,IR(0, 0),IR(15072, 19)                        ,IR(64, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {6017,IR(0, 0),IR(15113, 22)                           ,IR(65, 1),IR(109, 1),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {6018,IR(0, 0),IR(15160, 28)                                 ,IR(66, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {6019,IR(0, 0),IR(15219, 11)                ,IR(67, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {6020,IR(0, 0),IR(9148, 15)                    ,IR(0, 0),IR(1, 0),IR(32, 1),0xffffffffu,0xffffffffu},
  {6022,IR(0, 0),IR(10351, 21)                          ,IR(0, 0),IR(1, 0),IR(154, 1),0xffffffffu,0xffffffffu},
  {6024,IR(0, 0),IR(15244, 24)                             ,IR(0, 0),IR(1, 0),IR(155, 1),0xffffffffu,0xffffffffu},
  {6025,IR(0, 0),IR(15268, 16)                     ,IR(0, 0),IR(1, 0),IR(156, 1),0xffffffffu,0xffffffffu},
  {6026,IR(0, 0),IR(15284, 25)                              ,IR(0, 0),IR(77, 1),IR(157, 1),0xffffffffu,0xffffffffu},
  {6029,IR(0, 0),IR(15309, 15)                    ,IR(0, 0),IR(1, 0),IR(158, 1),0xffffffffu,0xffffffffu},
  {6030,IR(0, 0),IR(15324, 7)            ,IR(0, 0),IR(1, 0),IR(159, 1),0xffffffffu,0xffffffffu},
  {6032,IR(0, 0),IR(15331, 42)                                               ,IR(0, 0),IR(3, 1),IR(160, 1),0xffffffffu,0xffffffffu},
  {6033,IR(0, 0),IR(15373, 20)                         ,IR(0, 0),IR(1, 0),IR(161, 1),0xffffffffu,0xffffffffu},
  {6034,IR(0, 0),IR(15393, 20)                         ,IR(0, 0),IR(1, 0),IR(161, 1),0xffffffffu,0xffffffffu},
  {6089,IR(0, 0),IR(15413, 20)                         ,IR(0, 0),IR(1, 0),IR(162, 1),0xffffffffu,0xffffffffu},
  {6094,IR(0, 0),IR(15433, 11)                ,IR(68, 1),IR(1, 0),IR(163, 2),0xffffffffu,0xffffffffu},
  {6095,IR(0, 0),IR(15457, 20)                         ,IR(0, 0),IR(1, 0),IR(165, 1),0xffffffffu,0xffffffffu},
  {6114,IR(0, 0),IR(15477, 21)                          ,IR(0, 0),IR(1, 0),IR(166, 1),0xffffffffu,0xffffffffu},
  {6115,IR(0, 0),IR(15498, 24)                             ,IR(0, 0),IR(1, 0),IR(167, 1),0xffffffffu,0xffffffffu},
  {6141,IR(0, 0),IR(15522, 16)                     ,IR(69, 1),IR(1, 0),IR(168, 2),0xffffffffu,0xffffffffu},
  {6144,IR(0, 0),IR(15556, 19)                        ,IR(0, 0),IR(1, 0),IR(170, 1),0xffffffffu,0xffffffffu},
  {6150,IR(0, 0),IR(15575, 30)                                   ,IR(70, 1),IR(110, 1),IR(130, 2),0xffffffffu,0xffffffffu},
  {6161,IR(0, 0),IR(15634, 28)                                 ,IR(0, 0),IR(111, 1),IR(127, 1),0xffffffffu,0xffffffffu},
  {6162,IR(0, 0),IR(15662, 19)                        ,IR(71, 1),IR(1, 0),IR(171, 2),0xffffffffu,0xffffffffu},
  {6169,IR(0, 0),IR(15699, 16)                     ,IR(0, 0),IR(1, 0),IR(173, 1),0xffffffffu,0xffffffffu},
  {6171,IR(0, 0),IR(15715, 25)                              ,IR(72, 1),IR(1, 0),IR(174, 2),0xffffffffu,0xffffffffu},
  {6174,IR(0, 0),IR(15764, 29)                                  ,IR(73, 1),IR(1, 0),IR(176, 2),0xffffffffu,0xffffffffu},
  {6187,IR(0, 0),IR(15821, 30)                                   ,IR(0, 0),IR(1, 0),IR(178, 1),0xffffffffu,0xffffffffu},
  {6189,IR(0, 0),IR(15851, 36)                                         ,IR(74, 1),IR(1, 0),IR(179, 2),0xffffffffu,0xffffffffu},
  {6220,IR(0, 0),IR(15922, 28)                                 ,IR(0, 0),IR(1, 0),IR(181, 1),0xffffffffu,0xffffffffu},
  {6228,IR(0, 0),IR(15950, 23)                            ,IR(0, 0),IR(1, 0),IR(182, 1),0xffffffffu,0xffffffffu},
  {6229,IR(0, 0),IR(15973, 30)                                   ,IR(0, 0),IR(112, 1),IR(182, 1),0xffffffffu,0xffffffffu},
  {6230,IR(0, 0),IR(16003, 30)                                   ,IR(0, 0),IR(112, 1),IR(182, 1),0xffffffffu,0xffffffffu},
  {6236,IR(0, 0),IR(16033, 38)                                           ,IR(0, 0),IR(1, 0),IR(183, 1),0xffffffffu,0xffffffffu},
  {6241,IR(0, 0),IR(16071, 28)                                 ,IR(0, 0),IR(1, 0),IR(184, 1),0xffffffffu,0xffffffffu},
  {6243,IR(0, 0),IR(16099, 32)                                     ,IR(0, 0),IR(113, 2),IR(116, 1),0xffffffffu,0xffffffffu},
  {6245,IR(0, 0),IR(16131, 21)                          ,IR(0, 0),IR(1, 0),IR(185, 1),0xffffffffu,0xffffffffu},
  {6246,IR(0, 0),IR(16152, 22)                           ,IR(0, 0),IR(115, 1),IR(185, 1),0xffffffffu,0xffffffffu},
  {6257,IR(0, 0),IR(16174, 18)                       ,IR(0, 0),IR(1, 0),IR(186, 1),0xffffffffu,0xffffffffu},
  {6265,IR(0, 0),IR(16192, 23)                            ,IR(0, 0),IR(1, 0),IR(187, 1),0xffffffffu,0xffffffffu},
  {6400,IR(0, 0),IR(16215, 26)                               ,IR(0, 0),IR(1, 0),IR(188, 1),0xffffffffu,0xffffffffu},
  {6425,IR(0, 0),IR(16241, 27)                                ,IR(0, 0),IR(1, 0),IR(189, 1),0xffffffffu,0xffffffffu},
  {6427,IR(0, 0),IR(16268, 25)                              ,IR(0, 0),IR(1, 0),IR(190, 1),0xffffffffu,0xffffffffu},
  {6441,IR(0, 0),IR(16293, 19)                        ,IR(0, 0),IR(1, 0),IR(191, 1),0xffffffffu,0xffffffffu},
  {6460,IR(0, 0),IR(16312, 20)                         ,IR(0, 0),IR(1, 0),IR(192, 1),0xffffffffu,0xffffffffu},
  {6528,IR(0, 0),IR(16332, 20)                         ,IR(0, 0),IR(1, 0),IR(193, 1),0xffffffffu,0xffffffffu},
  {6912,IR(0, 0),IR(16352, 33)                                      ,IR(0, 0),IR(116, 1),IR(194, 1),0xffffffffu,0xffffffffu},
  {6913,IR(0, 0),IR(16385, 33)                                      ,IR(0, 0),IR(116, 1),IR(194, 1),0xffffffffu,0xffffffffu},
  {6914,IR(0, 0),IR(16418, 27)                                ,IR(0, 0),IR(90, 1),IR(194, 1),0xffffffffu,0xffffffffu},
  {6915,IR(0, 0),IR(16445, 32)                                     ,IR(0, 0),IR(117, 1),IR(194, 1),0xffffffffu,0xffffffffu},
  {7041,IR(0, 0),IR(16477, 17)                      ,IR(0, 0),IR(1, 0),IR(195, 1),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16494, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16506, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16514, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(16522, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(0, 0),IR(16528, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {5,IR(0, 0),IR(16535, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {6,IR(0, 0),IR(16546, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {7,IR(0, 0),IR(16555, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16568, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16574, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16584, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16590, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16605, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16630, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16646, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(16660, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(0, 0),IR(16673, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(0, 0),IR(16685, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {16,IR(0, 0),IR(16702, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {32,IR(0, 0),IR(16714, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {64,IR(0, 0),IR(16729, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {128,IR(0, 0),IR(16742, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {256,IR(0, 0),IR(16757, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {512,IR(0, 0),IR(16775, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1024,IR(0, 0),IR(16792, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2048,IR(0, 0),IR(16813, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4096,IR(0, 0),IR(16833, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8192,IR(0, 0),IR(16853, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {16384,IR(0, 0),IR(16869, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {32768,IR(0, 0),IR(16885, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {65536,IR(0, 0),IR(16905, 24)                             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16929, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16935, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16940, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 1),IR(16979, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(1, 2),IR(16990, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {5,IR(0, 0),IR(16999, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {6,IR(0, 0),IR(17004, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {7,IR(0, 0),IR(17011, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(0, 1),IR(17022, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {9,IR(1, 2),IR(17029, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(17038, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(17048, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(17061, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(17074, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(17085, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17095, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(17105, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(17115, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(17128, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {5,IR(0, 0),IR(17142, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {6,IR(0, 0),IR(17156, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {7,IR(0, 0),IR(17170, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(0, 0),IR(17185, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {9,IR(0, 0),IR(17201, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {10,IR(0, 0),IR(17217, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1000491000,IR(0, 0),IR(17233, 19)                        ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1000491001,IR(0, 0),IR(17252, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1000491002,IR(0, 0),IR(17273, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1000491003,IR(0, 0),IR(17285, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(17297, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17309, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4202,IR(0, 0),IR(17324, 25)                              ,IR(0, 0),IR(118, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4203,IR(0, 0),IR(17349, 28)                                 ,IR(0, 0),IR(118, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(17377, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17385, 27)                                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(17412, 27)                                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(17439, 27)                                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(0, 0),IR(17466, 32)                                     ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {16,IR(0, 0),IR(17498, 26)                               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17524, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(17528, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(17535, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(17539, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17550, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(17561, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(17582, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17593, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(17607, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(17628, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16494, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16506, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16514, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(0, 0),IR(16522, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {5,IR(0, 0),IR(16528, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {6,IR(0, 0),IR(16535, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {7,IR(0, 0),IR(16546, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(0, 0),IR(16555, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16568, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16574, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16584, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16630, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16646, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(16660, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(0, 0),IR(16673, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(0, 0),IR(16685, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {16,IR(0, 0),IR(16702, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {32,IR(0, 0),IR(16714, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {64,IR(0, 0),IR(16729, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {128,IR(0, 0),IR(16742, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {256,IR(0, 0),IR(16757, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {512,IR(0, 0),IR(16775, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1024,IR(0, 0),IR(16792, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2048,IR(0, 0),IR(16813, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4096,IR(0, 0),IR(16833, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8192,IR(0, 0),IR(16853, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16929, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16935, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16940, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 1),IR(16979, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(1, 2),IR(16990, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {5,IR(0, 0),IR(16999, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {6,IR(0, 0),IR(17004, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {7,IR(0, 0),IR(17011, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(0, 1),IR(17022, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(17038, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(17048, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(17061, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(17646, 17)                      ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 1),IR(17663, 7)            ,IR(0, 0),IR(119, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(17670, 6)           ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(17676, 12)                 ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),SPV_SPIRV_VERSION_WORD(1,3)},
  {4,IR(0, 0),IR(17688, 9)              ,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(17697, 9)              ,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 1),IR(17706, 12)                 ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 1),IR(17718, 13)                  ,IR(0, 0),IR(67, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(17731, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(17742, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(17753, 8)             ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(3, 1),IR(17787, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(17795, 14)                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(17809, 5)          ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(17814, 6)           ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(17820, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(0, 0),IR(17829, 7)            ,IR(0, 0),IR(25, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {18,IR(0, 0),IR(17836, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(17846, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {20,IR(0, 0),IR(17855, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {21,IR(0, 0),IR(17863, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {22,IR(0, 0),IR(17872, 9)              ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {23,IR(0, 0),IR(17881, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {24,IR(0, 0),IR(17890, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {25,IR(0, 0),IR(17902, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {26,IR(0, 0),IR(17914, 8)             ,IR(0, 0),IR(121, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {27,IR(4, 1),IR(17948, 10)               ,IR(0, 0),IR(121, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {28,IR(0, 0),IR(17958, 20)                         ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {29,IR(0, 1),IR(17978, 7)            ,IR(0, 0),IR(123, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {30,IR(0, 1),IR(17985, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {31,IR(0, 1),IR(17994, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {32,IR(0, 1),IR(18004, 6)           ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {33,IR(0, 1),IR(18010, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {34,IR(0, 1),IR(18018, 14)                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {35,IR(0, 1),IR(18032, 7)            ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {36,IR(0, 1),IR(18039, 10)               ,IR(0, 0),IR(124, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {37,IR(0, 1),IR(18049, 10)               ,IR(0, 0),IR(124, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {38,IR(5, 1),IR(18105, 14)                   ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {39,IR(6, 1),IR(18153, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {40,IR(7, 1),IR(18203, 15)                    ,IR(0, 0),IR(125, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {41,IR(8, 2),IR(18280, 18)                       ,IR(0, 0),IR(94, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {42,IR(0, 0),IR(18298, 14)                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {43,IR(0, 1),IR(18312, 21)                          ,IR(0, 0),IR(98, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {44,IR(0, 1),IR(18333, 10)               ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {45,IR(0, 1),IR(18343, 14)                   ,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {46,IR(10, 1),IR(18377, 12)                 ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu},
  {47,IR(10, 1),IR(18389, 16)                     ,IR(0, 0),IR(1, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu},
  {4216,IR(0, 0),IR(18405, 44)                                                 ,IR(0, 0),IR(117, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4469,IR(0, 0),IR(18449, 13)                  ,IR(0, 0),IR(1, 0),IR(196, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4470,IR(0, 0),IR(18462, 15)                    ,IR(0, 0),IR(1, 0),IR(196, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4487,IR(0, 0),IR(18477, 18)                       ,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu},
  {4488,IR(0, 0),IR(18495, 22)                           ,IR(0, 0),IR(1, 0),IR(53, 1),0xffffffffu,0xffffffffu},
  {4499,IR(0, 0),IR(18517, 22)                           ,IR(0, 0),IR(1, 0),IR(56, 1),0xffffffffu,0xffffffffu},
  {4999,IR(0, 0),IR(18539, 18)                       ,IR(0, 0),IR(1, 0),IR(10, 1),0xffffffffu,0xffffffffu},
  {5019,IR(10, 1),IR(18557, 32)                                     ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5020,IR(10, 1),IR(18589, 20)                         ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5078,IR(0, 0),IR(18609, 23)                            ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5091,IR(10, 1),IR(18632, 20)                         ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5098,IR(10, 1),IR(18652, 25)                              ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5099,IR(0, 0),IR(18677, 27)                                ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5100,IR(10, 1),IR(18704, 25)                              ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5105,IR(0, 0),IR(18729, 28)                                 ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5124,IR(10, 1),IR(18757, 17)                      ,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5125,IR(10, 1),IR(18774, 12)                 ,IR(0, 0),IR(44, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5145,IR(0, 0),IR(18786, 14)                   ,IR(0, 0),IR(127, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5248,IR(0, 0),IR(18800, 19)                        ,IR(0, 0),IR(128, 1),IR(74, 1),0xffffffffu,0xffffffffu},
  {5250,IR(0, 0),IR(18819, 14)                   ,IR(0, 0),IR(129, 1),IR(75, 1),0xffffffffu,0xffffffffu},
  {5252,IR(0, 0),IR(18833, 19)                        ,IR(0, 0),IR(97, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5256,IR(0, 1),IR(18852, 28)                                 ,IR(0, 0),IR(47, 1),IR(14, 1),0xffffffffu,0xffffffffu},
  {5271,IR(0, 0),IR(18880, 16)                     ,IR(75, 1),IR(130, 2),IR(197, 2),0xffffffffu,0xffffffffu},
  {5272,IR(0, 0),IR(18911, 10)               ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5273,IR(0, 0),IR(18921, 10)               ,IR(0, 0),IR(51, 1),IR(18, 1),0xffffffffu,0xffffffffu},
  {5285,IR(0, 0),IR(18931, 13)                  ,IR(76, 1),IR(52, 1),IR(19, 2),0xffffffffu,0xffffffffu},
  {5300,IR(0, 0),IR(18956, 11)                ,IR(77, 1),IR(100, 1),IR(85, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5355,IR(0, 0),IR(18981, 16)                     ,IR(78, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5356,IR(0, 0),IR(19016, 15)                    ,IR(79, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5358,IR(0, 1),IR(19049, 15)                    ,IR(0, 0),IR(132, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5386,IR(0, 0),IR(19064, 30)                                   ,IR(0, 0),IR(133, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5389,IR(0, 0),IR(19094, 31)                                    ,IR(0, 0),IR(134, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5397,IR(0, 1),IR(19125, 7)            ,IR(0, 0),IR(132, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5398,IR(0, 0),IR(19132, 18)                       ,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5399,IR(0, 0),IR(19150, 16)                     ,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5400,IR(0, 0),IR(19166, 15)                    ,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5401,IR(0, 0),IR(19181, 13)                  ,IR(0, 0),IR(135, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5599,IR(0, 1),IR(19194, 14)                   ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5602,IR(0, 0),IR(19208, 26)                               ,IR(0, 0),IR(137, 1),IR(110, 1),0xffffffffu,0xffffffffu},
  {5607,IR(11, 1),IR(19234, 13)                  ,IR(0, 0),IR(138, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5608,IR(0, 0),IR(19247, 17)                      ,IR(0, 0),IR(138, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5624,IR(0, 0),IR(19264, 27)                                ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5625,IR(0, 1),IR(19291, 21)                          ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5626,IR(0, 0),IR(19312, 27)                                ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5627,IR(0, 0),IR(19339, 15)                    ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5628,IR(0, 1),IR(19354, 26)                               ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5634,IR(10, 1),IR(19380, 14)                   ,IR(80, 1),IR(1, 0),IR(199, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {5635,IR(11, 1),IR(19418, 13)                  ,IR(81, 1),IR(1, 0),IR(199, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {5636,IR(11, 1),IR(19450, 15)                    ,IR(0, 0),IR(1, 0),IR(200, 1),0xffffffffu,0xffffffffu},
  {5822,IR(12, 2),IR(19465, 26)                               ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5823,IR(14, 2),IR(19522, 24)                             ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5825,IR(0, 0),IR(19546, 15)                    ,IR(82, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5826,IR(11, 1),IR(19575, 13)                  ,IR(83, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5827,IR(0, 1),IR(19600, 15)                    ,IR(84, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5828,IR(0, 1),IR(19629, 16)                     ,IR(85, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5829,IR(0, 1),IR(19660, 23)                            ,IR(86, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5830,IR(0, 0),IR(19705, 17)                      ,IR(87, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5831,IR(0, 0),IR(19738, 17)                      ,IR(88, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5832,IR(0, 1),IR(19771, 20)                         ,IR(89, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5833,IR(0, 0),IR(19810, 21)                          ,IR(90, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5834,IR(16, 2),IR(19851, 12)                 ,IR(91, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5835,IR(18, 1),IR(19916, 15)                    ,IR(92, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5836,IR(0, 1),IR(19945, 21)                          ,IR(93, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5883,IR(0, 1),IR(19986, 17)                      ,IR(94, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5884,IR(0, 1),IR(20019, 15)                    ,IR(95, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5885,IR(0, 0),IR(20048, 19)                        ,IR(96, 1),IR(140, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5899,IR(0, 0),IR(20085, 20)                         ,IR(97, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5900,IR(0, 1),IR(20124, 16)                     ,IR(98, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5901,IR(0, 0),IR(20155, 29)                                  ,IR(99, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5902,IR(0, 1),IR(20212, 15)                    ,IR(100, 1),IR(141, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5905,IR(0, 0),IR(20241, 18)                       ,IR(101, 1),IR(110, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5907,IR(0, 0),IR(20276, 26)                               ,IR(102, 1),IR(142, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5909,IR(1, 2),IR(20327, 20)                         ,IR(103, 1),IR(143, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5914,IR(10, 1),IR(20366, 16)                     ,IR(0, 0),IR(144, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5915,IR(10, 1),IR(20382, 13)                  ,IR(0, 0),IR(144, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5917,IR(0, 1),IR(20395, 25)                              ,IR(104, 1),IR(145, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5918,IR(0, 1),IR(20444, 21)                          ,IR(105, 1),IR(145, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5919,IR(0, 1),IR(20485, 21)                          ,IR(106, 1),IR(145, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5921,IR(0, 1),IR(20526, 21)                          ,IR(107, 1),IR(146, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5944,IR(0, 1),IR(20567, 20)                         ,IR(108, 1),IR(147, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6080,IR(19, 2),IR(20640, 31)                                    ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6085,IR(0, 0),IR(20671, 25)                              ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6087,IR(0, 0),IR(20696, 35)                                        ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6140,IR(0, 0),IR(20731, 18)                       ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6151,IR(0, 0),IR(20749, 16)                     ,IR(109, 1),IR(148, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6170,IR(21, 1),IR(20811, 26)                               ,IR(0, 0),IR(149, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6172,IR(0, 1),IR(20837, 26)                               ,IR(110, 1),IR(150, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6173,IR(22, 3),IR(20888, 31)                                    ,IR(111, 1),IR(150, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6175,IR(0, 0),IR(20949, 28)                                 ,IR(112, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6176,IR(0, 0),IR(21004, 32)                                     ,IR(113, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6177,IR(0, 1),IR(21067, 34)                                       ,IR(114, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6178,IR(0, 1),IR(21134, 31)                                    ,IR(115, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6179,IR(0, 1),IR(21195, 29)                                  ,IR(116, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6180,IR(25, 1),IR(21286, 35)                                        ,IR(117, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6181,IR(0, 1),IR(21355, 30)                                   ,IR(118, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6182,IR(0, 1),IR(21414, 33)                                      ,IR(119, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6183,IR(0, 0),IR(21479, 27)                                ,IR(120, 1),IR(151, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6188,IR(26, 2),IR(21571, 16)                     ,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6190,IR(28, 1),IR(21634, 15)                    ,IR(121, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6191,IR(0, 1),IR(21663, 29)                                  ,IR(122, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6247,IR(10, 1),IR(21720, 17)                      ,IR(0, 0),IR(115, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6442,IR(29, 2),IR(21773, 22)                           ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6443,IR(31, 2),IR(21832, 23)                            ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {7040,IR(0, 1),IR(16477, 17)                      ,IR(0, 0),IR(155, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(21855, 3)        ,IR(0, 0),IR(73, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(21858, 3)        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(21861, 3)        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(21864, 5)          ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(21869, 5)          ,IR(0, 0),IR(72, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(21874, 7)            ,IR(0, 0),IR(74, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(21881, 12)                 ,IR(0, 0),IR(98, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4173,IR(0, 0),IR(21893, 17)                      ,IR(0, 0),IR(156, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 1),IR(21910, 12)                 ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(21922, 13)                  ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(21935, 22)                           ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(21957, 21)                          ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(21978, 14)                   ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(21992, 15)                    ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(22007, 19)                        ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(22026, 16)                     ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(22042, 16)                     ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(22058, 19)                        ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(22077, 10)               ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(22087, 4)         ,IR(0, 0),IR(124, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(22091, 15)                    ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(22106, 13)                  ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(22119, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(22129, 15)                    ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(22, 3),IR(22144, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {18,IR(22, 3),IR(22154, 14)                   ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(22168, 12)                 ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {20,IR(0, 0),IR(22180, 11)                ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {21,IR(0, 0),IR(22191, 20)                         ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {22,IR(0, 0),IR(22211, 10)               ,IR(0, 0),IR(12, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {23,IR(0, 0),IR(22221, 24)                             ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {24,IR(0, 0),IR(22245, 6)           ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {25,IR(0, 0),IR(22251, 9)              ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {26,IR(0, 1),IR(22260, 15)                    ,IR(0, 0),IR(157, 4),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {27,IR(0, 0),IR(22275, 13)                  ,IR(0, 0),IR(161, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {28,IR(0, 0),IR(22288, 16)                     ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {29,IR(0, 0),IR(22304, 20)                         ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {30,IR(0, 1),IR(22324, 12)                 ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {31,IR(0, 0),IR(22336, 15)                    ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {33,IR(0, 0),IR(22351, 12)                 ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {34,IR(0, 0),IR(22363, 10)               ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {35,IR(0, 1),IR(6579, 13)                  ,IR(0, 0),IR(164, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {36,IR(0, 1),IR(22373, 22)                           ,IR(0, 0),IR(164, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {37,IR(10, 1),IR(22395, 24)                             ,IR(0, 0),IR(164, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu},
  {38,IR(33, 3),IR(22419, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu},
  {39,IR(33, 3),IR(22431, 16)                     ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,2),0xffffffffu},
  {4169,IR(0, 0),IR(22447, 34)                                       ,IR(0, 0),IR(156, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4170,IR(0, 0),IR(22481, 34)                                       ,IR(0, 0),IR(165, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4171,IR(0, 0),IR(22515, 36)                                         ,IR(0, 0),IR(166, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4421,IR(0, 0),IR(22551, 30)                                   ,IR(0, 0),IR(3, 1),IR(201, 1),0xffffffffu,0xffffffffu},
  {4446,IR(0, 0),IR(22581, 18)                       ,IR(0, 0),IR(167, 1),IR(46, 1),0xffffffffu,0xffffffffu},
  {4459,IR(0, 1),IR(10897, 15)                    ,IR(0, 0),IR(168, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4460,IR(0, 1),IR(10912, 18)                       ,IR(0, 0),IR(169, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4461,IR(0, 1),IR(10930, 25)                              ,IR(0, 0),IR(170, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4462,IR(0, 1),IR(10955, 16)                     ,IR(0, 0),IR(171, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4463,IR(0, 1),IR(10971, 16)                     ,IR(0, 0),IR(172, 1),IR(48, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {4489,IR(0, 0),IR(22599, 34)                                       ,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4490,IR(22, 3),IR(22633, 20)                         ,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5017,IR(0, 0),IR(22653, 29)                                  ,IR(0, 0),IR(3, 1),IR(202, 1),0xffffffffu,0xffffffffu},
  {5027,IR(0, 0),IR(22682, 23)                            ,IR(0, 0),IR(42, 1),IR(11, 1),0xffffffffu,0xffffffffu},
  {5069,IR(0, 0),IR(22705, 15)                    ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5070,IR(10, 1),IR(22720, 15)                    ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5071,IR(10, 1),IR(22735, 21)                          ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5072,IR(33, 3),IR(22756, 24)                             ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5073,IR(10, 1),IR(7472, 16)                     ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5077,IR(33, 3),IR(22780, 21)                          ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5079,IR(0, 0),IR(22801, 28)                                 ,IR(0, 0),IR(42, 1),IR(203, 2),0xffffffffu,0xffffffffu},
  {5080,IR(0, 0),IR(22829, 26)                               ,IR(0, 0),IR(42, 1),IR(203, 2),0xffffffffu,0xffffffffu},
  {5081,IR(0, 0),IR(22855, 23)                            ,IR(0, 0),IR(42, 1),IR(203, 2),0xffffffffu,0xffffffffu},
  {5082,IR(0, 0),IR(22878, 27)                                ,IR(0, 0),IR(42, 1),IR(203, 2),0xffffffffu,0xffffffffu},
  {5083,IR(0, 0),IR(22905, 25)                              ,IR(0, 0),IR(42, 1),IR(203, 2),0xffffffffu,0xffffffffu},
  {5084,IR(0, 0),IR(22930, 22)                           ,IR(0, 0),IR(42, 1),IR(203, 2),0xffffffffu,0xffffffffu},
  {5088,IR(0, 0),IR(22952, 19)                        ,IR(0, 0),IR(173, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5089,IR(0, 0),IR(22971, 20)                         ,IR(0, 0),IR(173, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5102,IR(36, 2),IR(22991, 20)                         ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5157,IR(0, 0),IR(23011, 20)                         ,IR(0, 0),IR(174, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5269,IR(0, 0),IR(23031, 15)                    ,IR(123, 1),IR(130, 2),IR(197, 2),0xffffffffu,0xffffffffu},
  {5270,IR(0, 1),IR(23060, 20)                         ,IR(124, 1),IR(130, 2),IR(197, 2),0xffffffffu,0xffffffffu},
  {5289,IR(0, 0),IR(23099, 24)                             ,IR(125, 1),IR(175, 1),IR(81, 2),0xffffffffu,0xffffffffu},
  {5290,IR(0, 0),IR(23146, 25)                              ,IR(126, 1),IR(176, 1),IR(81, 2),0xffffffffu,0xffffffffu},
  {5298,IR(0, 0),IR(23195, 19)                        ,IR(127, 1),IR(130, 2),IR(197, 2),0xffffffffu,0xffffffffu},
  {5366,IR(0, 0),IR(23232, 25)                              ,IR(0, 0),IR(177, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5367,IR(0, 0),IR(23257, 27)                                ,IR(0, 0),IR(177, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5368,IR(0, 0),IR(23284, 26)                               ,IR(0, 0),IR(178, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5369,IR(0, 0),IR(23310, 28)                                 ,IR(0, 0),IR(178, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5370,IR(0, 0),IR(23338, 31)                                    ,IR(0, 0),IR(179, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5371,IR(0, 0),IR(23369, 33)                                      ,IR(0, 0),IR(179, 1),IR(89, 1),0xffffffffu,0xffffffffu},
  {5427,IR(0, 0),IR(13362, 23)                            ,IR(0, 0),IR(180, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5618,IR(0, 1),IR(23402, 27)                                ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5620,IR(0, 1),IR(23429, 21)                          ,IR(0, 0),IR(181, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5621,IR(0, 1),IR(23450, 21)                          ,IR(0, 0),IR(181, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5622,IR(0, 1),IR(23471, 26)                               ,IR(0, 0),IR(181, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5623,IR(0, 1),IR(23497, 27)                                ,IR(0, 0),IR(181, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5893,IR(22, 3),IR(23524, 22)                           ,IR(0, 0),IR(182, 1),IR(127, 1),0xffffffffu,0xffffffffu},
  {5894,IR(0, 1),IR(23546, 16)                     ,IR(0, 0),IR(182, 1),IR(127, 1),0xffffffffu,0xffffffffu},
  {5895,IR(0, 0),IR(23562, 20)                         ,IR(0, 0),IR(182, 1),IR(127, 1),0xffffffffu,0xffffffffu},
  {5896,IR(0, 1),IR(23582, 22)                           ,IR(0, 0),IR(111, 1),IR(127, 1),0xffffffffu,0xffffffffu},
  {5903,IR(0, 1),IR(23604, 28)                                 ,IR(0, 0),IR(111, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6023,IR(0, 0),IR(23632, 24)                             ,IR(0, 0),IR(3, 1),IR(205, 1),0xffffffffu,0xffffffffu},
  {6028,IR(36, 2),IR(23656, 18)                       ,IR(0, 0),IR(183, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6031,IR(10, 1),IR(23674, 21)                          ,IR(0, 0),IR(184, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6154,IR(0, 1),IR(23695, 24)                             ,IR(0, 0),IR(111, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6160,IR(0, 1),IR(23719, 26)                               ,IR(0, 0),IR(185, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6417,IR(0, 1),IR(23745, 23)                            ,IR(0, 0),IR(136, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6461,IR(0, 1),IR(23768, 22)                           ,IR(0, 0),IR(186, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6462,IR(10, 1),IR(23790, 24)                             ,IR(0, 0),IR(186, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6463,IR(38, 1),IR(23865, 27)                                ,IR(0, 0),IR(186, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(23892, 7)            ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(23899, 20)                         ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(23919, 23)                            ,IR(0, 0),IR(24, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(6060, 9)              ,IR(0, 0),IR(70, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(17029, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(23942, 10)               ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(5827, 7)            ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5267,IR(0, 0),IR(23952, 7)            ,IR(0, 0),IR(51, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5268,IR(0, 0),IR(23959, 7)            ,IR(0, 0),IR(51, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5313,IR(0, 0),IR(23966, 17)                      ,IR(128, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {5314,IR(0, 0),IR(23999, 16)                     ,IR(129, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {5315,IR(0, 0),IR(24030, 10)               ,IR(130, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {5316,IR(0, 0),IR(24049, 14)                   ,IR(131, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {5317,IR(0, 0),IR(24076, 8)             ,IR(132, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {5318,IR(0, 0),IR(24091, 12)                 ,IR(133, 1),IR(55, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {5364,IR(0, 0),IR(24114, 8)             ,IR(0, 0),IR(54, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5365,IR(0, 0),IR(24122, 8)             ,IR(0, 0),IR(54, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24130, 9)              ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(24139, 12)                 ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24151, 12)                 ,IR(0, 0),IR(90, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4214,IR(0, 0),IR(24163, 14)                   ,IR(0, 0),IR(117, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4215,IR(0, 0),IR(24177, 14)                   ,IR(0, 0),IR(117, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4223,IR(0, 0),IR(24191, 14)                   ,IR(0, 0),IR(187, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4224,IR(0, 0),IR(24205, 14)                   ,IR(0, 0),IR(187, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4225,IR(0, 0),IR(24219, 14)                   ,IR(0, 0),IR(188, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4226,IR(0, 0),IR(10392, 22)                           ,IR(0, 0),IR(189, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4227,IR(0, 0),IR(10414, 10)               ,IR(0, 0),IR(190, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24233, 5)          ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(24238, 4)         ,IR(0, 0),IR(139, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(24242, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(24249, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(24256, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(24260, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(24271, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {65536,IR(0, 0),IR(24276, 14)                   ,IR(134, 1),IR(191, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {131072,IR(0, 0),IR(24313, 13)                  ,IR(135, 1),IR(191, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {262144,IR(0, 0),IR(24344, 15)                    ,IR(0, 0),IR(183, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24359, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(24363, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(24367, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(24371, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(24375, 16)                     ,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(24391, 16)                     ,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(24407, 18)                       ,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(0, 0),IR(24425, 18)                       ,IR(0, 0),IR(38, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(24443, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(24450, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(24461, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(24466, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {65536,IR(0, 0),IR(15433, 11)                ,IR(68, 1),IR(193, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24472, 5)          ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(24477, 5)          ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(24482, 6)           ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(24488, 5)          ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(24493, 8)             ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(24501, 10)               ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(24511, 8)             ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(24519, 12)                 ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5940,IR(0, 0),IR(24531, 21)                          ,IR(136, 1),IR(194, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(24572, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(24586, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(24598, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(24611, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24624, 7)            ,IR(0, 0),IR(195, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(24631, 14)                   ,IR(0, 0),IR(195, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(24645, 14)                   ,IR(0, 0),IR(195, 3),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(24659, 16)                     ,IR(0, 0),IR(198, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {6,IR(0, 0),IR(24675, 21)                          ,IR(137, 1),IR(199, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {7,IR(0, 0),IR(24716, 28)                                 ,IR(138, 1),IR(199, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(0, 0),IR(24771, 28)                                 ,IR(139, 1),IR(199, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(24826, 10)               ,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(24836, 10)               ,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(24846, 11)                ,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(24857, 15)                    ,IR(0, 0),IR(152, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(10, 1),IR(24872, 5)          ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(10, 1),IR(24877, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(36, 2),IR(24881, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(10, 1),IR(24886, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(10, 1),IR(18032, 7)            ,IR(0, 0),IR(200, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {32,IR(10, 1),IR(24898, 13)                  ,IR(0, 0),IR(200, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {64,IR(10, 1),IR(17829, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {128,IR(10, 1),IR(9660, 7)            ,IR(0, 0),IR(201, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {256,IR(4, 1),IR(24911, 19)                        ,IR(140, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {512,IR(4, 1),IR(24952, 17)                      ,IR(141, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {1024,IR(0, 0),IR(24989, 16)                     ,IR(142, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {2048,IR(0, 0),IR(25024, 14)                   ,IR(143, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {4096,IR(0, 0),IR(25055, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {8192,IR(0, 0),IR(25066, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {16384,IR(0, 0),IR(25077, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {65536,IR(10, 1),IR(25089, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(25097, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(25107, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(25118, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(25128, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(25139, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(25153, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(25167, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(25182, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(25193, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(25205, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(25217, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(25230, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(25244, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(25258, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(16522, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(25268, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(25279, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(0, 0),IR(25296, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(25312, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {20,IR(0, 0),IR(25332, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {21,IR(0, 0),IR(25352, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {22,IR(0, 0),IR(25372, 19)                        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {23,IR(0, 0),IR(25391, 19)                        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {24,IR(0, 0),IR(25410, 19)                        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {25,IR(0, 0),IR(25429, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {26,IR(0, 0),IR(25445, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(25461, 2)       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(25463, 2)       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(25465, 3)        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(25468, 3)        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(25471, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(25475, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(25480, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(25485, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(25490, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(25500, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(25510, 3)        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(25513, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(25517, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(25522, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(25528, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(25541, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(25546, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(0, 0),IR(25552, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {18,IR(0, 0),IR(25558, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(25564, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(25569, 28)                                 ,IR(144, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(25624, 24)                             ,IR(145, 1),IR(153, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(25671, 7)            ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(25678, 11)                ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(25689, 14)                   ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(25703, 12)                 ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(25715, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(25728, 7)            ,IR(0, 0),IR(94, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(25735, 7)            ,IR(0, 0),IR(94, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(25742, 12)                 ,IR(0, 0),IR(94, 1),IR(206, 1),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(25754, 8)             ,IR(0, 0),IR(203, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(25762, 14)                   ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(25776, 12)                 ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(25788, 15)                    ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(25803, 25)                              ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(25828, 17)                      ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(25845, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(25852, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(25863, 19)                        ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {8,IR(0, 1),IR(25882, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,1),0xffffffffu},
  {16,IR(0, 1),IR(25899, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {32,IR(0, 1),IR(25913, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {64,IR(0, 1),IR(25927, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {128,IR(0, 1),IR(25945, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {256,IR(0, 1),IR(25955, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {65536,IR(0, 1),IR(20395, 25)                              ,IR(104, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {131072,IR(0, 1),IR(20444, 21)                          ,IR(105, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {262144,IR(0, 1),IR(25968, 22)                           ,IR(146, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {524288,IR(0, 1),IR(20485, 21)                          ,IR(106, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1048576,IR(0, 1),IR(26011, 19)                        ,IR(147, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2097152,IR(0, 1),IR(26048, 22)                           ,IR(148, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4194304,IR(0, 1),IR(26091, 27)                                ,IR(149, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {8388608,IR(0, 0),IR(26144, 15)                    ,IR(150, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {16777216,IR(0, 1),IR(26173, 16)                     ,IR(151, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {33554432,IR(0, 1),IR(26204, 27)                                ,IR(152, 1),IR(204, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {268435456,IR(0, 1),IR(11197, 23)                            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(26257, 29)                                  ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(26286, 29)                                  ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(26315, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(0, 0),IR(26336, 26)                               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {16,IR(0, 0),IR(26362, 23)                            ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {32,IR(0, 0),IR(26385, 23)                            ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {64,IR(0, 0),IR(26408, 23)                            ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {128,IR(0, 0),IR(26431, 23)                            ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {256,IR(0, 0),IR(26454, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {512,IR(0, 0),IR(26471, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1024,IR(0, 0),IR(26488, 26)                               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2048,IR(0, 0),IR(26514, 26)                               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4096,IR(0, 0),IR(26540, 27)                                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {8192,IR(0, 0),IR(26567, 27)                                ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(17863, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 1),IR(26594, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(25077, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(4, 1),IR(26602, 21)                          ,IR(153, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {16,IR(4, 1),IR(26647, 19)                        ,IR(154, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {32,IR(0, 0),IR(26688, 18)                       ,IR(155, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {65536,IR(10, 1),IR(26727, 20)                         ,IR(0, 0),IR(144, 1),IR(136, 1),0xffffffffu,0xffffffffu},
  {131072,IR(10, 1),IR(26747, 17)                      ,IR(0, 0),IR(144, 1),IR(136, 1),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(26764, 7)            ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(26771, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(26779, 7)            ,IR(0, 0),IR(0, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(26786, 7)            ,IR(156, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {0,IR(0, 0),IR(26803, 8)             ,IR(157, 1),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(26811, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(26819, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(26827, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(26842, 23)                            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {64,IR(0, 0),IR(26865, 14)                   ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {128,IR(0, 0),IR(26879, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {256,IR(0, 0),IR(26894, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {512,IR(0, 0),IR(26910, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1024,IR(0, 0),IR(26931, 20)                         ,IR(0, 0),IR(83, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2048,IR(0, 0),IR(26951, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4096,IR(0, 0),IR(26963, 13)                  ,IR(158, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {8192,IR(0, 0),IR(26992, 14)                   ,IR(159, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {16384,IR(0, 0),IR(27023, 12)                 ,IR(160, 1),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {32768,IR(0, 0),IR(17863, 9)              ,IR(0, 0),IR(202, 1),IR(87, 1),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {0,IR(0, 0),IR(27050, 10)               ,IR(0, 0),IR(186, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(27060, 5)          ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(27065, 4)         ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(27069, 9)              ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(27078, 8)             ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(27086, 25)                              ,IR(161, 1),IR(1, 0),IR(153, 1),SPV_SPIRV_VERSION_WORD(1,6),0xffffffffu},
  {0,IR(0, 0),IR(27139, 4)         ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(27143, 9)              ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(27152, 4)         ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(27156, 9)              ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(27165, 8)             ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5,IR(0, 0),IR(27173, 12)                 ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {6,IR(0, 0),IR(27185, 9)              ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {7,IR(0, 0),IR(27194, 13)                  ,IR(0, 0),IR(205, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(27207, 25)                              ,IR(0, 0),IR(206, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(27232, 23)                            ,IR(0, 0),IR(206, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(17377, 8)             ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(27255, 10)               ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(27265, 12)                 ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(27277, 23)                            ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(0, 0),IR(27300, 24)                             ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {16,IR(0, 0),IR(27324, 27)                                ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {32,IR(0, 0),IR(27351, 28)                                 ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {64,IR(0, 0),IR(27379, 14)                   ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {128,IR(0, 0),IR(27393, 16)                     ,IR(0, 0),IR(85, 2),IR(0, 0),0xffffffffu,0xffffffffu},
  {256,IR(0, 0),IR(27409, 17)                      ,IR(162, 1),IR(207, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {512,IR(0, 0),IR(27450, 13)                  ,IR(0, 0),IR(207, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1024,IR(0, 0),IR(27463, 30)                                   ,IR(163, 1),IR(208, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(27523, 41)                                              ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(27564, 37)                                          ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(27601, 37)                                          ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(27638, 41)                                              ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(27679, 42)                                               ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(27721, 33)                                      ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(27754, 33)                                      ,IR(0, 0),IR(209, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(27787, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(27799, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(27805, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(27812, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(27827, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(27835, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(27842, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(27850, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(27858, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(27866, 5)          ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(27871, 6)           ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(27877, 11)                ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(27888, 6)           ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(27894, 6)           ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(27900, 13)                  ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(27913, 5)          ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(27918, 7)            ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(27925, 8)             ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(27933, 5)          ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(27938, 4)         ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(27942, 4)         ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(27946, 3)        ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {16,IR(0, 0),IR(27949, 12)                 ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {17,IR(0, 0),IR(27961, 10)               ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {18,IR(0, 0),IR(27971, 9)              ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {19,IR(0, 0),IR(27980, 9)              ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {20,IR(0, 0),IR(27989, 8)             ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {21,IR(0, 0),IR(27997, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {22,IR(0, 0),IR(28005, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {23,IR(0, 0),IR(28013, 7)            ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {24,IR(0, 0),IR(28020, 5)          ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {25,IR(0, 0),IR(28025, 6)           ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {26,IR(0, 0),IR(28031, 6)           ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {27,IR(0, 0),IR(28037, 5)          ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {28,IR(0, 0),IR(28042, 5)          ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {29,IR(0, 0),IR(28047, 4)         ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {30,IR(0, 0),IR(28051, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {31,IR(0, 0),IR(28060, 9)              ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {32,IR(0, 0),IR(28069, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {33,IR(0, 0),IR(28077, 6)           ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {34,IR(0, 0),IR(28083, 10)               ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {35,IR(0, 0),IR(28093, 7)            ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {36,IR(0, 0),IR(28100, 7)            ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {37,IR(0, 0),IR(28107, 6)           ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {38,IR(0, 0),IR(28113, 6)           ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {39,IR(0, 0),IR(28119, 5)          ,IR(0, 0),IR(210, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {40,IR(0, 0),IR(28124, 6)           ,IR(0, 0),IR(211, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {41,IR(0, 0),IR(28130, 5)          ,IR(0, 0),IR(211, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(28135, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(28147, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(28154, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(28164, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(28173, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(28184, 12)                 ,IR(164, 1),IR(202, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {6,IR(0, 0),IR(28211, 14)                   ,IR(0, 0),IR(61, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(28225, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(28233, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(28245, 29)                                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16494, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16506, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16514, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(16522, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(0, 0),IR(16528, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {5,IR(0, 0),IR(16535, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {6,IR(0, 0),IR(16546, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {7,IR(0, 0),IR(16555, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16568, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16574, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16584, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16590, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16605, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16630, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16646, 14)                   ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(16660, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(0, 0),IR(16673, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(0, 0),IR(16685, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {16,IR(0, 0),IR(16702, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {32,IR(0, 0),IR(16714, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {64,IR(0, 0),IR(16729, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {128,IR(0, 0),IR(16742, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {256,IR(0, 0),IR(16757, 18)                       ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {512,IR(0, 0),IR(16775, 17)                      ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1024,IR(0, 0),IR(16792, 21)                          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2048,IR(0, 0),IR(16813, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4096,IR(0, 0),IR(16833, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8192,IR(0, 0),IR(16853, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {16384,IR(0, 0),IR(16869, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {32768,IR(0, 0),IR(16885, 20)                         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {65536,IR(0, 0),IR(16905, 24)                             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {131072,IR(0, 0),IR(28274, 26)                               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(16929, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(16935, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(16940, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(10, 1),IR(16979, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {4,IR(36, 2),IR(16990, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {5,IR(0, 0),IR(16999, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {6,IR(0, 0),IR(17004, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {7,IR(0, 0),IR(17011, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {8,IR(10, 1),IR(17022, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {9,IR(36, 2),IR(17029, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(17038, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {1,IR(0, 0),IR(17048, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {2,IR(0, 0),IR(17061, 13)                  ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {3,IR(0, 0),IR(17074, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1, 0),0xffffffffu},
  {0,IR(0, 0),IR(27842, 8)             ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(28300, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(28305, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(28310, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(28319, 11)                ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(28330, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(28335, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(28350, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(28355, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(28362, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(28367, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(28372, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(28378, 4)         ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {13,IR(0, 0),IR(28382, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {14,IR(0, 0),IR(28387, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {15,IR(0, 0),IR(28392, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {0,IR(0, 0),IR(28401, 16)                     ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {1,IR(0, 0),IR(28417, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {2,IR(0, 0),IR(17914, 8)             ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {3,IR(0, 0),IR(28423, 7)            ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {4,IR(0, 0),IR(28154, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {5,IR(0, 0),IR(28430, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {6,IR(0, 0),IR(28445, 8)             ,IR(0, 0),IR(212, 2),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {7,IR(0, 0),IR(28453, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {8,IR(0, 0),IR(28462, 8)             ,IR(0, 0),IR(214, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {9,IR(0, 0),IR(28470, 13)                  ,IR(0, 0),IR(3, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {10,IR(0, 0),IR(28483, 14)                   ,IR(0, 0),IR(83, 1),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {11,IR(0, 0),IR(28497, 6)           ,IR(0, 0),IR(1, 0),IR(0, 0),SPV_SPIRV_VERSION_WORD(1,0),0xffffffffu},
  {12,IR(0, 0),IR(28503, 14)                   ,IR(0, 0),IR(3, 1),IR(207, 2),SPV_SPIRV_VERSION_WORD(1,3),0xffffffffu},
  {4172,IR(0, 0),IR(28517, 13)                  ,IR(0, 0),IR(156, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4491,IR(0, 0),IR(28530, 19)                        ,IR(0, 0),IR(41, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5068,IR(0, 0),IR(28549, 16)                     ,IR(0, 0),IR(43, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5328,IR(0, 0),IR(28565, 16)                     ,IR(165, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5329,IR(0, 0),IR(28596, 24)                             ,IR(166, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5338,IR(0, 0),IR(28643, 14)                   ,IR(167, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5339,IR(0, 0),IR(28670, 16)                     ,IR(168, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5342,IR(0, 0),IR(28701, 22)                           ,IR(169, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5343,IR(0, 0),IR(28744, 22)                           ,IR(170, 1),IR(55, 2),IR(24, 2),0xffffffffu,0xffffffffu},
  {5349,IR(0, 0),IR(28787, 22)                           ,IR(171, 1),IR(2, 1),IR(0, 2),SPV_SPIRV_VERSION_WORD(1,5),0xffffffffu},
  {5385,IR(0, 0),IR(28834, 21)                          ,IR(0, 0),IR(133, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5402,IR(0, 0),IR(28855, 24)                             ,IR(0, 0),IR(54, 1),IR(23, 1),SPV_SPIRV_VERSION_WORD(1,4),0xffffffffu},
  {5411,IR(0, 0),IR(28879, 22)                           ,IR(0, 0),IR(134, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5605,IR(0, 0),IR(28901, 17)                      ,IR(0, 0),IR(215, 1),IR(110, 1),0xffffffffu,0xffffffffu},
  {5936,IR(0, 0),IR(28918, 17)                      ,IR(172, 1),IR(216, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {5937,IR(0, 0),IR(28951, 15)                    ,IR(173, 1),IR(216, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(25762, 14)                   ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(28980, 18)                       ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(28998, 15)                    ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(25788, 15)                    ,IR(0, 0),IR(154, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(16625, 5)          ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(10, 1),IR(29013, 11)                ,IR(0, 0),IR(217, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(10, 1),IR(29024, 11)                ,IR(0, 0),IR(107, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(10, 1),IR(29035, 17)                      ,IR(0, 0),IR(218, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(29052, 10)               ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(17872, 9)              ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(0, 0),IR(27787, 12)                 ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {3,IR(0, 0),IR(27805, 7)            ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(0, 0),IR(27812, 15)                    ,IR(0, 0),IR(1, 0),IR(0, 0),0xffffffffu,0xffffffffu},
  {0,IR(0, 0),IR(29062, 8)             ,IR(0, 0),IR(219, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {1,IR(0, 0),IR(29070, 15)                    ,IR(0, 0),IR(219, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {2,IR(10, 1),IR(29085, 20)                         ,IR(0, 0),IR(219, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {4,IR(10, 1),IR(29105, 24)                             ,IR(0, 0),IR(219, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {8,IR(10, 1),IR(29129, 22)                           ,IR(0, 0),IR(219, 1),IR(0, 0),0xffffffffu,0xffffffffu},
  {16,IR(0, 0),IR(29151, 21)                          ,IR(0, 0),IR(219, 1),IR(0, 0),0xffffffffu,0xffffffffu},
}};






IndexRange OperandByValueRangeForKind(spv_operand_type_t type) {
  switch(type) {
    case SPV_OPERAND_TYPE_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_ADDRESSING_MODEL: return IR(3, 4);
    case SPV_OPERAND_TYPE_BUILT_IN: return IR(7, 128);
    case SPV_OPERAND_TYPE_CAPABILITY: return IR(135, 301);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(436, 8);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(444, 3);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(447, 2);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS: return IR(449, 19);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION: return IR(468, 10);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(478, 4);
    case SPV_OPERAND_TYPE_COMPONENT_TYPE: return IR(482, 15);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT: return IR(497, 4);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS: return IR(501, 6);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE: return IR(507, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE: return IR(510, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT: return IR(513, 4);
    case SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(517, 8);
    case SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE: return IR(525, 3);
    case SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS: return IR(528, 16);
    case SPV_OPERAND_TYPE_DEBUG_OPERATION: return IR(544, 9);
    case SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER: return IR(553, 3);
    case SPV_OPERAND_TYPE_DECORATION: return IR(556, 151);
    case SPV_OPERAND_TYPE_DIMENSIONALITY: return IR(707, 8);
    case SPV_OPERAND_TYPE_EXECUTION_MODE: return IR(715, 99);
    case SPV_OPERAND_TYPE_EXECUTION_MODEL: return IR(814, 17);
    case SPV_OPERAND_TYPE_FPDENORM_MODE: return IR(831, 2);
    case SPV_OPERAND_TYPE_FPENCODING: return IR(833, 8);
    case SPV_OPERAND_TYPE_FPOPERATION_MODE: return IR(841, 2);
    case SPV_OPERAND_TYPE_FP_FAST_MATH_MODE: return IR(843, 9);
    case SPV_OPERAND_TYPE_FP_ROUNDING_MODE: return IR(852, 4);
    case SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE: return IR(856, 4);
    case SPV_OPERAND_TYPE_FUNCTION_CONTROL: return IR(860, 6);
    case SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE: return IR(866, 9);
    case SPV_OPERAND_TYPE_GATHER_MODES: return IR(875, 4);
    case SPV_OPERAND_TYPE_GROUP_OPERATION: return IR(879, 7);
    case SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER: return IR(886, 4);
    case SPV_OPERAND_TYPE_IMAGE: return IR(890, 17);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE: return IR(907, 26);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER: return IR(933, 20);
    case SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER: return IR(953, 2);
    case SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS: return IR(955, 3);
    case SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO: return IR(958, 2);
    case SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS: return IR(960, 2);
    case SPV_OPERAND_TYPE_LINKAGE_TYPE: return IR(962, 4);
    case SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL: return IR(966, 5);
    case SPV_OPERAND_TYPE_LOOP_CONTROL: return IR(971, 21);
    case SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(992, 15);
    case SPV_OPERAND_TYPE_MEMORY_ACCESS: return IR(1007, 9);
    case SPV_OPERAND_TYPE_MEMORY_MODEL: return IR(1016, 4);
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID: return IR(1020, 15);
    case SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS: return IR(1035, 1);
    case SPV_OPERAND_TYPE_OVERFLOW_MODES: return IR(1036, 4);
    case SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT: return IR(1040, 1);
    case SPV_OPERAND_TYPE_QUANTIZATION_MODES: return IR(1041, 8);
    case SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS: return IR(1049, 3);
    case SPV_OPERAND_TYPE_RAY_FLAGS: return IR(1052, 12);
    case SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE: return IR(1064, 2);
    case SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE: return IR(1066, 3);
    case SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION: return IR(1069, 2);
    case SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE: return IR(1071, 5);
    case SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE: return IR(1076, 2);
    case SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT: return IR(1078, 42);
    case SPV_OPERAND_TYPE_SCOPE_ID: return IR(1120, 7);
    case SPV_OPERAND_TYPE_SELECTION_CONTROL: return IR(1127, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_BUILD_IDENTIFIER_FLAGS: return IR(1130, 1);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(1131, 8);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(1139, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(1142, 2);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS: return IR(1144, 20);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION: return IR(1164, 10);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(1174, 4);
    case SPV_OPERAND_TYPE_SOURCE_LANGUAGE: return IR(1178, 16);
    case SPV_OPERAND_TYPE_STORAGE_CLASS: return IR(1194, 29);
    case SPV_OPERAND_TYPE_STORE_CACHE_CONTROL: return IR(1223, 4);
    case SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS: return IR(1227, 4);
    case SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE: return IR(1231, 5);
    case SPV_OPERAND_TYPE_TENSOR_OPERANDS: return IR(1236, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_IMAGE: return IR(890, 17);
    case SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS: return IR(1007, 9);
    case SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT: return IR(1040, 1);
    case SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS: return IR(501, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(992, 15);
    case SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS: return IR(1049, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_FPENCODING: return IR(833, 8);
    case SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS: return IR(1236, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_CAPABILITY: return IR(135, 301);
    default: break;
  }
  return IR(0,0);
}





static const std::array<NameIndex, 1420> kOperandNames{{
  {IR(5818, 9), 0},
  {IR(5844, 10), 2},
  {IR(5834, 10), 1},
  {IR(5854, 8), 3},
  {IR(5862, 11), 4},
  {IR(5883, 11), 5},
  {IR(5894, 24), 6},
  {IR(5918, 27), 6},
  {IR(7877, 13), 94},
  {IR(7890, 12), 94},
  {IR(7223, 20), 68},
  {IR(7243, 28), 69},
  {IR(7925, 20), 95},
  {IR(7945, 19), 95},
  {IR(7271, 26), 70},
  {IR(7368, 22), 74},
  {IR(7297, 19), 71},
  {IR(7316, 27), 72},
  {IR(7343, 25), 73},
  {IR(7027, 13), 59},
  {IR(7001, 11), 58},
  {IR(6002, 13), 9},
  {IR(7784, 22), 89},
  {IR(9083, 12), 133},
  {IR(6765, 13), 49},
  {IR(6739, 10), 48},
  {IR(6778, 13), 50},
  {IR(6015, 13), 10},
  {IR(7806, 22), 90},
  {IR(9136, 12), 134},
  {IR(8132, 17), 101},
  {IR(8536, 17), 115},
  {IR(7097, 12), 62},
  {IR(7040, 10), 60},
  {IR(6529, 22), 37},
  {IR(6299, 10), 21},
  {IR(6385, 10), 27},
  {IR(8010, 23), 97},
  {IR(7964, 12), 96},
  {IR(7390, 18), 75},
  {IR(7976, 15), 96},
  {IR(6320, 12), 23},
  {IR(7696, 16), 85},
  {IR(6470, 19), 33},
  {IR(6564, 15), 39},
  {IR(6551, 13), 38},
  {IR(6518, 11), 36},
  {IR(6395, 17), 28},
  {IR(8838, 11), 122},
  {IR(8796, 14), 121},
  {IR(9018, 33), 130},
  {IR(8984, 34), 129},
  {IR(8515, 11), 114},
  {IR(8526, 10), 114},
  {IR(8966, 18), 128},
  {IR(9069, 14), 132},
  {IR(8700, 37), 118},
  {IR(8633, 34), 117},
  {IR(8888, 20), 123},
  {IR(9051, 18), 131},
  {IR(8508, 7), 113},
  {IR(8576, 30), 116},
  {IR(8737, 20), 119},
  {IR(8757, 19), 119},
  {IR(8397, 23), 110},
  {IR(8420, 22), 110},
  {IR(6037, 11), 12},
  {IR(6725, 14), 47},
  {IR(6138, 13), 14},
  {IR(8033, 22), 97},
  {IR(8149, 12), 102},
  {IR(8161, 11), 102},
  {IR(8172, 14), 103},
  {IR(8186, 13), 103},
  {IR(6151, 6), 15},
  {IR(7828, 15), 91},
  {IR(6452, 18), 32},
  {IR(6489, 21), 34},
  {IR(7843, 16), 92},
  {IR(7859, 18), 93},
  {IR(6655, 21), 43},
  {IR(6642, 13), 42},
  {IR(6412, 14), 29},
  {IR(8312, 22), 107},
  {IR(8334, 21), 107},
  {IR(8275, 19), 106},
  {IR(8294, 18), 106},
  {IR(8442, 17), 111},
  {IR(8459, 16), 111},
  {IR(6285, 14), 20},
  {IR(6309, 11), 22},
  {IR(5992, 10), 8},
  {IR(5976, 9), 7},
  {IR(7636, 18), 83},
  {IR(7748, 17), 87},
  {IR(6048, 12), 13},
  {IR(7765, 19), 88},
  {IR(8080, 24), 99},
  {IR(8055, 25), 98},
  {IR(7050, 24), 61},
  {IR(8104, 28), 100},
  {IR(8776, 20), 120},
  {IR(8376, 11), 109},
  {IR(8387, 10), 109},
  {IR(8355, 11), 108},
  {IR(8366, 10), 108},
  {IR(7425, 29), 76},
  {IR(7521, 16), 79},
  {IR(8940, 10), 125},
  {IR(8959, 7), 127},
  {IR(6332, 9), 24},
  {IR(6374, 11), 26},
  {IR(6359, 15), 25},
  {IR(7488, 15), 78},
  {IR(7573, 20), 81},
  {IR(7612, 24), 82},
  {IR(7472, 16), 77},
  {IR(7141, 15), 64},
  {IR(6814, 15), 53},
  {IR(6829, 18), 53},
  {IR(6869, 15), 54},
  {IR(6884, 18), 54},
  {IR(6902, 15), 55},
  {IR(6917, 18), 55},
  {IR(6676, 11), 44},
  {IR(6935, 15), 56},
  {IR(6950, 18), 56},
  {IR(6687, 26), 45},
  {IR(6968, 15), 57},
  {IR(6983, 18), 57},
  {IR(6626, 16), 41},
  {IR(6579, 13), 40},
  {IR(7736, 12), 86},
  {IR(6275, 10), 19},
  {IR(6260, 15), 18},
  {IR(6245, 15), 17},
  {IR(7205, 18), 67},
  {IR(7187, 18), 66},
  {IR(7156, 15), 65},
  {IR(6028, 9), 11},
  {IR(6713, 12), 46},
  {IR(7121, 10), 63},
  {IR(6197, 14), 16},
  {IR(7537, 15), 80},
  {IR(7674, 22), 84},
  {IR(6791, 10), 51},
  {IR(8950, 9), 126},
  {IR(6801, 13), 52},
  {IR(8908, 13), 124},
  {IR(6510, 8), 35},
  {IR(6440, 12), 31},
  {IR(6426, 14), 30},
  {IR(8234, 21), 105},
  {IR(8255, 20), 105},
  {IR(8199, 18), 104},
  {IR(8217, 17), 104},
  {IR(8475, 17), 112},
  {IR(8492, 16), 112},
  {IR(11498, 9), 274},
  {IR(5873, 10), 139},
  {IR(14804, 35), 379},
  {IR(14839, 34), 379},
  {IR(14276, 38), 367},
  {IR(14314, 37), 367},
  {IR(14211, 33), 366},
  {IR(14244, 32), 366},
  {IR(15556, 19), 405},
  {IR(13849, 9), 352},
  {IR(15457, 20), 401},
  {IR(13904, 23), 355},
  {IR(13288, 22), 326},
  {IR(15373, 20), 397},
  {IR(13858, 23), 353},
  {IR(15393, 20), 398},
  {IR(13881, 23), 354},
  {IR(9316, 14), 155},
  {IR(10773, 17), 236},
  {IR(15498, 24), 403},
  {IR(11469, 29), 273},
  {IR(11447, 22), 272},
  {IR(11431, 16), 271},
  {IR(16332, 20), 430},
  {IR(13225, 18), 323},
  {IR(15268, 16), 392},
  {IR(10424, 18), 220},
  {IR(15006, 20), 383},
  {IR(15026, 19), 383},
  {IR(16293, 19), 428},
  {IR(6002, 13), 165},
  {IR(12827, 32), 311},
  {IR(12859, 31), 311},
  {IR(11677, 31), 290},
  {IR(11708, 30), 290},
  {IR(11507, 16), 276},
  {IR(13522, 30), 338},
  {IR(11142, 32), 255},
  {IR(13415, 31), 335},
  {IR(13599, 32), 342},
  {IR(10351, 21), 390},
  {IR(10286, 28), 213},
  {IR(12915, 20), 313},
  {IR(13446, 40), 336},
  {IR(13385, 30), 334},
  {IR(13486, 36), 337},
  {IR(13268, 20), 325},
  {IR(13552, 28), 339},
  {IR(6749, 16), 205},
  {IR(6015, 13), 166},
  {IR(15477, 21), 402},
  {IR(13038, 25), 318},
  {IR(13063, 28), 318},
  {IR(10912, 18), 242},
  {IR(10897, 15), 241},
  {IR(9763, 18), 184},
  {IR(7503, 18), 275},
  {IR(9287, 14), 153},
  {IR(7109, 12), 232},
  {IR(13091, 23), 319},
  {IR(15219, 11), 388},
  {IR(16418, 27), 433},
  {IR(16385, 33), 432},
  {IR(16352, 33), 431},
  {IR(16445, 32), 434},
  {IR(15113, 22), 386},
  {IR(15135, 25), 386},
  {IR(15160, 28), 387},
  {IR(15188, 31), 387},
  {IR(15072, 19), 385},
  {IR(15091, 22), 385},
  {IR(15230, 14), 388},
  {IR(7012, 15), 223},
  {IR(13961, 16), 358},
  {IR(15324, 7), 395},
  {IR(14191, 20), 365},
  {IR(15764, 29), 411},
  {IR(15793, 28), 411},
  {IR(14755, 25), 378},
  {IR(14780, 24), 378},
  {IR(14523, 28), 373},
  {IR(14551, 27), 373},
  {IR(15575, 30), 406},
  {IR(15605, 29), 406},
  {IR(14607, 21), 375},
  {IR(14628, 20), 375},
  {IR(14674, 41), 377},
  {IR(14715, 40), 377},
  {IR(14448, 26), 371},
  {IR(15634, 28), 407},
  {IR(15715, 25), 410},
  {IR(15740, 24), 410},
  {IR(14381, 23), 369},
  {IR(14404, 22), 369},
  {IR(14474, 25), 372},
  {IR(14499, 24), 372},
  {IR(14138, 27), 364},
  {IR(14165, 26), 364},
  {IR(15045, 14), 384},
  {IR(15059, 13), 384},
  {IR(15699, 16), 409},
  {IR(9201, 8), 144},
  {IR(9187, 14), 143},
  {IR(11271, 16), 260},
  {IR(10382, 10), 217},
  {IR(9209, 8), 145},
  {IR(10372, 10), 216},
  {IR(10324, 27), 215},
  {IR(10314, 10), 214},
  {IR(10392, 22), 218},
  {IR(15309, 15), 394},
  {IR(13757, 23), 348},
  {IR(7902, 23), 289},
  {IR(11655, 22), 289},
  {IR(7991, 19), 291},
  {IR(7712, 24), 285},
  {IR(11309, 16), 262},
  {IR(13006, 32), 317},
  {IR(12935, 33), 314},
  {IR(12968, 38), 315},
  {IR(7074, 23), 221},
  {IR(14112, 26), 363},
  {IR(13803, 22), 350},
  {IR(16152, 22), 422},
  {IR(9608, 15), 171},
  {IR(6060, 9), 137},
  {IR(9358, 18), 158},
  {IR(11583, 28), 280},
  {IR(9821, 16), 187},
  {IR(15851, 36), 413},
  {IR(15887, 35), 413},
  {IR(15821, 30), 412},
  {IR(10277, 9), 212},
  {IR(6592, 16), 194},
  {IR(9960, 26), 196},
  {IR(6847, 22), 197},
  {IR(10040, 25), 200},
  {IR(11752, 30), 292},
  {IR(11782, 29), 292},
  {IR(10065, 20), 201},
  {IR(15284, 25), 393},
  {IR(9986, 23), 198},
  {IR(10009, 31), 199},
  {IR(9940, 20), 195},
  {IR(16215, 26), 425},
  {IR(9280, 7), 152},
  {IR(14979, 14), 382},
  {IR(14993, 13), 382},
  {IR(9677, 8), 177},
  {IR(9236, 11), 148},
  {IR(9699, 12), 180},
  {IR(9554, 15), 167},
  {IR(11638, 17), 287},
  {IR(11287, 22), 261},
  {IR(9376, 20), 159},
  {IR(11242, 29), 259},
  {IR(11220, 22), 258},
  {IR(9711, 13), 181},
  {IR(9262, 12), 150},
  {IR(9752, 11), 183},
  {IR(9247, 15), 149},
  {IR(11325, 21), 264},
  {IR(9586, 10), 169},
  {IR(13825, 24), 351},
  {IR(9628, 16), 173},
  {IR(11897, 36), 295},
  {IR(11933, 39), 295},
  {IR(12438, 39), 302},
  {IR(12477, 42), 302},
  {IR(9330, 6), 156},
  {IR(11404, 27), 270},
  {IR(11390, 14), 269},
  {IR(9217, 6), 146},
  {IR(9223, 13), 147},
  {IR(11346, 14), 265},
  {IR(9623, 5), 172},
  {IR(13780, 23), 349},
  {IR(9781, 22), 185},
  {IR(16477, 17), 435},
  {IR(5827, 7), 141},
  {IR(14426, 22), 370},
  {IR(9170, 8), 140},
  {IR(9301, 15), 154},
  {IR(15413, 20), 399},
  {IR(13348, 14), 332},
  {IR(14578, 15), 374},
  {IR(14593, 14), 374},
  {IR(10414, 10), 219},
  {IR(16268, 25), 427},
  {IR(9163, 7), 135},
  {IR(14648, 26), 376},
  {IR(6123, 15), 288},
  {IR(6109, 14), 286},
  {IR(9660, 7), 175},
  {IR(7131, 10), 233},
  {IR(6211, 14), 190},
  {IR(11197, 23), 257},
  {IR(9915, 13), 192},
  {IR(15433, 11), 400},
  {IR(15444, 13), 400},
  {IR(7654, 20), 284},
  {IR(5945, 31), 310},
  {IR(12793, 34), 310},
  {IR(9928, 12), 193},
  {IR(9274, 6), 151},
  {IR(11523, 16), 277},
  {IR(16174, 18), 423},
  {IR(13328, 20), 331},
  {IR(11375, 15), 268},
  {IR(13310, 18), 328},
  {IR(9148, 15), 389},
  {IR(11010, 12), 247},
  {IR(13243, 25), 324},
  {IR(10987, 23), 246},
  {IR(9095, 41), 340},
  {IR(8667, 33), 327},
  {IR(6095, 14), 250},
  {IR(8849, 39), 330},
  {IR(8553, 23), 307},
  {IR(6082, 13), 306},
  {IR(13143, 29), 320},
  {IR(15331, 42), 396},
  {IR(13114, 29), 320},
  {IR(8606, 27), 305},
  {IR(12890, 25), 312},
  {IR(8810, 28), 329},
  {IR(11041, 32), 249},
  {IR(16312, 20), 429},
  {IR(15244, 24), 391},
  {IR(13736, 21), 347},
  {IR(16192, 23), 424},
  {IR(10955, 16), 244},
  {IR(10971, 16), 245},
  {IR(14920, 30), 381},
  {IR(14950, 29), 381},
  {IR(11848, 23), 294},
  {IR(11871, 26), 294},
  {IR(11554, 29), 279},
  {IR(10790, 28), 237},
  {IR(6341, 18), 168},
  {IR(9667, 10), 176},
  {IR(9685, 14), 179},
  {IR(9569, 17), 178},
  {IR(9454, 33), 162},
  {IR(12211, 36), 299},
  {IR(12247, 39), 299},
  {IR(9596, 12), 170},
  {IR(5985, 7), 136},
  {IR(13362, 23), 333},
  {IR(11360, 15), 266},
  {IR(7454, 18), 267},
  {IR(13198, 27), 322},
  {IR(13172, 26), 321},
  {IR(6157, 12), 202},
  {IR(11811, 17), 293},
  {IR(11828, 20), 293},
  {IR(8921, 19), 316},
  {IR(7593, 19), 283},
  {IR(6225, 20), 203},
  {IR(6169, 28), 281},
  {IR(11611, 27), 281},
  {IR(7552, 21), 282},
  {IR(11738, 14), 291},
  {IR(10930, 25), 243},
  {IR(9644, 16), 174},
  {IR(16131, 21), 421},
  {IR(15522, 16), 404},
  {IR(15538, 18), 404},
  {IR(7408, 17), 263},
  {IR(10578, 25), 228},
  {IR(10818, 24), 238},
  {IR(9487, 34), 163},
  {IR(12286, 37), 300},
  {IR(12323, 40), 300},
  {IR(9521, 33), 164},
  {IR(12363, 36), 301},
  {IR(12399, 39), 301},
  {IR(9724, 28), 182},
  {IR(9396, 24), 160},
  {IR(9837, 30), 188},
  {IR(9867, 31), 189},
  {IR(10705, 21), 231},
  {IR(10683, 22), 230},
  {IR(10876, 21), 240},
  {IR(10200, 37), 210},
  {IR(10237, 40), 211},
  {IR(12053, 39), 297},
  {IR(12092, 42), 297},
  {IR(12606, 42), 304},
  {IR(12648, 45), 304},
  {IR(10666, 17), 229},
  {IR(10603, 28), 228},
  {IR(15950, 23), 415},
  {IR(15973, 30), 416},
  {IR(16003, 30), 417},
  {IR(14048, 39), 361},
  {IR(13977, 33), 359},
  {IR(14010, 38), 360},
  {IR(6608, 18), 222},
  {IR(13652, 27), 344},
  {IR(15922, 28), 414},
  {IR(9898, 17), 191},
  {IR(13679, 26), 345},
  {IR(13705, 31), 346},
  {IR(16033, 38), 418},
  {IR(13631, 21), 343},
  {IR(10562, 16), 227},
  {IR(15662, 19), 408},
  {IR(15681, 18), 408},
  {IR(13580, 19), 341},
  {IR(16241, 27), 426},
  {IR(10189, 11), 209},
  {IR(16071, 28), 419},
  {IR(6069, 13), 138},
  {IR(9336, 22), 157},
  {IR(11174, 23), 256},
  {IR(11120, 22), 253},
  {IR(11099, 21), 252},
  {IR(11073, 26), 251},
  {IR(10103, 28), 206},
  {IR(10131, 28), 207},
  {IR(10159, 30), 208},
  {IR(7171, 16), 254},
  {IR(9803, 18), 186},
  {IR(14873, 24), 380},
  {IR(14897, 23), 380},
  {IR(10631, 35), 229},
  {IR(10842, 34), 239},
  {IR(9420, 34), 161},
  {IR(12134, 37), 298},
  {IR(12171, 40), 298},
  {IR(10085, 18), 204},
  {IR(11972, 39), 296},
  {IR(12011, 42), 296},
  {IR(12519, 42), 303},
  {IR(12561, 45), 303},
  {IR(14351, 30), 368},
  {IR(11022, 19), 248},
  {IR(16099, 32), 420},
  {IR(14087, 25), 362},
  {IR(10756, 17), 235},
  {IR(10726, 30), 234},
  {IR(9178, 9), 142},
  {IR(13946, 15), 357},
  {IR(13927, 19), 356},
  {IR(12693, 18), 308},
  {IR(12732, 29), 309},
  {IR(12761, 32), 309},
  {IR(12711, 21), 308},
  {IR(11539, 15), 278},
  {IR(10518, 44), 226},
  {IR(10475, 43), 225},
  {IR(10442, 33), 224},
  {IR(16506, 8), 437},
  {IR(16514, 8), 438},
  {IR(16522, 6), 439},
  {IR(16528, 7), 440},
  {IR(16535, 11), 441},
  {IR(16546, 9), 442},
  {IR(16555, 13), 443},
  {IR(16494, 12), 436},
  {IR(16568, 6), 444},
  {IR(16574, 10), 445},
  {IR(16584, 6), 446},
  {IR(16605, 20), 448},
  {IR(16590, 15), 447},
  {IR(16714, 15), 456},
  {IR(16729, 13), 457},
  {IR(16702, 12), 455},
  {IR(16792, 21), 461},
  {IR(16685, 17), 454},
  {IR(16869, 16), 465},
  {IR(16673, 12), 453},
  {IR(16853, 16), 464},
  {IR(16646, 14), 451},
  {IR(16630, 16), 450},
  {IR(16660, 13), 452},
  {IR(16813, 20), 462},
  {IR(16757, 18), 459},
  {IR(16742, 15), 458},
  {IR(16833, 20), 463},
  {IR(16775, 17), 460},
  {IR(16905, 24), 467},
  {IR(16885, 20), 466},
  {IR(16625, 5), 449},
  {IR(16990, 9), 472},
  {IR(17022, 7), 476},
  {IR(16929, 6), 468},
  {IR(17029, 9), 477},
  {IR(16940, 6), 470},
  {IR(16935, 5), 469},
  {IR(16979, 11), 471},
  {IR(17011, 11), 475},
  {IR(16999, 5), 473},
  {IR(17004, 7), 474},
  {IR(17074, 11), 481},
  {IR(17038, 10), 478},
  {IR(17061, 13), 480},
  {IR(17048, 13), 479},
  {IR(17085, 10), 482},
  {IR(17095, 10), 483},
  {IR(17105, 10), 484},
  {IR(17273, 12), 495},
  {IR(17285, 12), 496},
  {IR(17128, 14), 486},
  {IR(17142, 14), 487},
  {IR(17156, 14), 488},
  {IR(17115, 13), 485},
  {IR(17233, 19), 493},
  {IR(17185, 16), 490},
  {IR(17201, 16), 491},
  {IR(17217, 16), 492},
  {IR(17170, 15), 489},
  {IR(17252, 21), 494},
  {IR(17349, 28), 500},
  {IR(17309, 15), 498},
  {IR(17324, 25), 499},
  {IR(17297, 12), 497},
  {IR(17385, 27), 502},
  {IR(17412, 27), 503},
  {IR(17439, 27), 504},
  {IR(17466, 32), 505},
  {IR(17377, 8), 501},
  {IR(17498, 26), 506},
  {IR(17535, 4), 509},
  {IR(17528, 7), 508},
  {IR(17524, 4), 507},
  {IR(17539, 11), 510},
  {IR(17561, 21), 512},
  {IR(17550, 11), 511},
  {IR(17593, 14), 514},
  {IR(17607, 21), 515},
  {IR(17582, 11), 513},
  {IR(17628, 18), 516},
  {IR(16506, 8), 518},
  {IR(16514, 8), 519},
  {IR(16522, 6), 520},
  {IR(16528, 7), 521},
  {IR(16535, 11), 522},
  {IR(16546, 9), 523},
  {IR(16555, 13), 524},
  {IR(16494, 12), 517},
  {IR(16568, 6), 525},
  {IR(16574, 10), 526},
  {IR(16584, 6), 527},
  {IR(16714, 15), 535},
  {IR(16729, 13), 536},
  {IR(16702, 12), 534},
  {IR(16792, 21), 540},
  {IR(16685, 17), 533},
  {IR(16673, 12), 532},
  {IR(16853, 16), 543},
  {IR(16646, 14), 530},
  {IR(16630, 16), 529},
  {IR(16660, 13), 531},
  {IR(16813, 20), 541},
  {IR(16757, 18), 538},
  {IR(16742, 15), 537},
  {IR(16833, 20), 542},
  {IR(16775, 17), 539},
  {IR(16625, 5), 528},
  {IR(16990, 9), 548},
  {IR(17022, 7), 552},
  {IR(16929, 6), 544},
  {IR(16940, 6), 546},
  {IR(16935, 5), 545},
  {IR(16979, 11), 547},
  {IR(17011, 11), 551},
  {IR(16999, 5), 549},
  {IR(17004, 7), 550},
  {IR(17038, 10), 553},
  {IR(17061, 13), 555},
  {IR(17048, 13), 554},
  {IR(20366, 16), 676},
  {IR(17855, 8), 575},
  {IR(19016, 15), 631},
  {IR(19031, 18), 631},
  {IR(18333, 10), 599},
  {IR(18377, 12), 601},
  {IR(17706, 12), 562},
  {IR(18757, 17), 618},
  {IR(19916, 15), 664},
  {IR(19931, 14), 664},
  {IR(19125, 7), 635},
  {IR(19629, 16), 657},
  {IR(19645, 15), 657},
  {IR(18010, 8), 588},
  {IR(19150, 16), 637},
  {IR(19132, 18), 636},
  {IR(17670, 6), 558},
  {IR(18517, 22), 608},
  {IR(18495, 22), 607},
  {IR(19181, 13), 639},
  {IR(19166, 15), 638},
  {IR(17676, 12), 559},
  {IR(20526, 21), 681},
  {IR(20547, 20), 681},
  {IR(17787, 8), 567},
  {IR(20085, 20), 669},
  {IR(20105, 19), 669},
  {IR(17753, 8), 566},
  {IR(21773, 22), 704},
  {IR(21832, 23), 705},
  {IR(20124, 16), 670},
  {IR(20140, 15), 670},
  {IR(17820, 9), 571},
  {IR(19234, 13), 642},
  {IR(17881, 9), 578},
  {IR(17697, 9), 561},
  {IR(17994, 10), 586},
  {IR(21720, 17), 703},
  {IR(20949, 28), 691},
  {IR(20977, 27), 691},
  {IR(17872, 9), 577},
  {IR(19380, 14), 649},
  {IR(18018, 14), 589},
  {IR(20155, 29), 671},
  {IR(20184, 28), 671},
  {IR(19738, 17), 660},
  {IR(19755, 16), 660},
  {IR(18539, 18), 609},
  {IR(18203, 15), 595},
  {IR(20811, 26), 688},
  {IR(18153, 15), 594},
  {IR(17809, 5), 569},
  {IR(19945, 21), 665},
  {IR(19966, 20), 665},
  {IR(18105, 14), 593},
  {IR(19291, 21), 645},
  {IR(19522, 24), 653},
  {IR(20640, 31), 683},
  {IR(19465, 26), 652},
  {IR(20276, 26), 674},
  {IR(20302, 25), 674},
  {IR(17742, 11), 565},
  {IR(17731, 11), 564},
  {IR(19354, 26), 648},
  {IR(19094, 31), 634},
  {IR(19064, 30), 633},
  {IR(19394, 24), 649},
  {IR(19431, 19), 650},
  {IR(21571, 16), 700},
  {IR(20567, 20), 682},
  {IR(20587, 19), 682},
  {IR(21663, 29), 702},
  {IR(21692, 28), 702},
  {IR(18004, 6), 587},
  {IR(21634, 15), 701},
  {IR(21649, 14), 701},
  {IR(20395, 25), 678},
  {IR(20420, 24), 678},
  {IR(18312, 21), 598},
  {IR(16477, 17), 706},
  {IR(17836, 10), 573},
  {IR(20888, 31), 690},
  {IR(20919, 30), 690},
  {IR(20837, 26), 689},
  {IR(20863, 25), 689},
  {IR(18280, 18), 596},
  {IR(17985, 9), 585},
  {IR(21067, 34), 693},
  {IR(21101, 33), 693},
  {IR(21134, 31), 694},
  {IR(21165, 30), 694},
  {IR(21195, 29), 695},
  {IR(21224, 28), 695},
  {IR(21355, 30), 697},
  {IR(21385, 29), 697},
  {IR(21286, 35), 696},
  {IR(21321, 34), 696},
  {IR(21414, 33), 698},
  {IR(21447, 32), 698},
  {IR(20327, 20), 675},
  {IR(20347, 19), 675},
  {IR(17718, 13), 563},
  {IR(18343, 14), 600},
  {IR(18389, 16), 602},
  {IR(20444, 21), 679},
  {IR(20465, 20), 679},
  {IR(19660, 23), 658},
  {IR(19683, 22), 658},
  {IR(19771, 20), 661},
  {IR(19791, 19), 661},
  {IR(20731, 18), 686},
  {IR(19049, 15), 632},
  {IR(19575, 13), 655},
  {IR(19588, 12), 655},
  {IR(19851, 12), 663},
  {IR(19863, 11), 663},
  {IR(20382, 13), 677},
  {IR(18298, 14), 597},
  {IR(17795, 14), 568},
  {IR(18449, 13), 604},
  {IR(18462, 15), 605},
  {IR(18589, 20), 611},
  {IR(18557, 32), 610},
  {IR(17902, 12), 580},
  {IR(18956, 11), 629},
  {IR(18967, 14), 629},
  {IR(17890, 12), 579},
  {IR(19600, 15), 656},
  {IR(19615, 14), 656},
  {IR(18032, 7), 590},
  {IR(18774, 12), 619},
  {IR(18800, 19), 621},
  {IR(18819, 14), 622},
  {IR(17814, 6), 570},
  {IR(18729, 28), 617},
  {IR(18704, 25), 616},
  {IR(18652, 25), 614},
  {IR(18632, 20), 613},
  {IR(18677, 27), 615},
  {IR(18880, 16), 625},
  {IR(18896, 15), 625},
  {IR(18921, 10), 627},
  {IR(18931, 13), 628},
  {IR(18944, 12), 628},
  {IR(18911, 10), 626},
  {IR(20485, 21), 680},
  {IR(20506, 20), 680},
  {IR(20212, 15), 672},
  {IR(20227, 14), 672},
  {IR(19208, 26), 641},
  {IR(19546, 15), 654},
  {IR(19561, 14), 654},
  {IR(21004, 32), 692},
  {IR(21036, 31), 692},
  {IR(17646, 17), 556},
  {IR(17846, 9), 574},
  {IR(18981, 16), 630},
  {IR(18997, 19), 630},
  {IR(17688, 9), 560},
  {IR(19194, 14), 640},
  {IR(17829, 7), 572},
  {IR(17958, 20), 583},
  {IR(18405, 44), 603},
  {IR(18852, 28), 624},
  {IR(19247, 17), 643},
  {IR(19810, 21), 662},
  {IR(19831, 20), 662},
  {IR(20671, 25), 684},
  {IR(19705, 17), 659},
  {IR(19722, 16), 659},
  {IR(17663, 7), 557},
  {IR(21479, 27), 699},
  {IR(21506, 26), 699},
  {IR(19339, 15), 647},
  {IR(20241, 18), 673},
  {IR(20259, 17), 673},
  {IR(20749, 16), 687},
  {IR(20765, 15), 687},
  {IR(17978, 7), 584},
  {IR(19986, 17), 666},
  {IR(20003, 16), 666},
  {IR(18609, 23), 612},
  {IR(20048, 19), 668},
  {IR(20067, 18), 668},
  {IR(18786, 14), 620},
  {IR(17914, 8), 581},
  {IR(17948, 10), 582},
  {IR(19418, 13), 650},
  {IR(19450, 15), 651},
  {IR(20696, 35), 685},
  {IR(19312, 27), 646},
  {IR(19264, 27), 644},
  {IR(18833, 19), 623},
  {IR(17863, 9), 576},
  {IR(18477, 18), 606},
  {IR(20019, 15), 667},
  {IR(20034, 14), 667},
  {IR(18039, 10), 591},
  {IR(18049, 10), 592},
  {IR(21855, 3), 707},
  {IR(21858, 3), 708},
  {IR(21861, 3), 709},
  {IR(21874, 7), 712},
  {IR(21864, 5), 710},
  {IR(21869, 5), 711},
  {IR(21881, 12), 713},
  {IR(21893, 17), 714},
  {IR(23011, 20), 782},
  {IR(22705, 15), 767},
  {IR(22336, 15), 745},
  {IR(10912, 18), 759},
  {IR(10897, 15), 758},
  {IR(22106, 13), 728},
  {IR(22119, 10), 729},
  {IR(22091, 15), 727},
  {IR(22129, 15), 730},
  {IR(23146, 25), 786},
  {IR(23171, 24), 786},
  {IR(23099, 24), 785},
  {IR(23123, 23), 785},
  {IR(22653, 29), 765},
  {IR(22058, 19), 724},
  {IR(23656, 18), 806},
  {IR(22363, 10), 747},
  {IR(23471, 26), 798},
  {IR(23497, 27), 799},
  {IR(22351, 12), 746},
  {IR(22180, 11), 734},
  {IR(22191, 20), 735},
  {IR(22168, 12), 733},
  {IR(22221, 24), 737},
  {IR(21910, 12), 715},
  {IR(22720, 15), 768},
  {IR(22251, 9), 739},
  {IR(22144, 10), 731},
  {IR(22154, 14), 732},
  {IR(22431, 16), 752},
  {IR(22419, 12), 751},
  {IR(22735, 21), 769},
  {IR(22780, 21), 772},
  {IR(23546, 16), 801},
  {IR(23524, 22), 800},
  {IR(23632, 24), 805},
  {IR(23768, 22), 811},
  {IR(23790, 24), 812},
  {IR(23745, 23), 810},
  {IR(23865, 27), 813},
  {IR(23562, 20), 802},
  {IR(22447, 34), 753},
  {IR(22481, 34), 754},
  {IR(22515, 36), 755},
  {IR(22599, 34), 763},
  {IR(23582, 22), 803},
  {IR(23674, 21), 807},
  {IR(22042, 16), 723},
  {IR(22026, 16), 722},
  {IR(22288, 16), 742},
  {IR(23031, 15), 783},
  {IR(23046, 14), 783},
  {IR(22275, 13), 741},
  {IR(23060, 20), 784},
  {IR(23080, 19), 784},
  {IR(22304, 20), 743},
  {IR(23195, 19), 787},
  {IR(23214, 18), 787},
  {IR(22260, 15), 740},
  {IR(22007, 19), 721},
  {IR(23232, 25), 788},
  {IR(23257, 27), 789},
  {IR(22077, 10), 725},
  {IR(22581, 18), 757},
  {IR(22952, 19), 779},
  {IR(22245, 6), 738},
  {IR(23719, 26), 809},
  {IR(22971, 20), 780},
  {IR(10955, 16), 761},
  {IR(23450, 21), 797},
  {IR(23429, 21), 796},
  {IR(10971, 16), 762},
  {IR(23284, 26), 790},
  {IR(23310, 28), 791},
  {IR(23604, 28), 804},
  {IR(13362, 23), 794},
  {IR(7472, 16), 771},
  {IR(23338, 31), 792},
  {IR(23369, 33), 793},
  {IR(23402, 27), 795},
  {IR(22991, 20), 781},
  {IR(10930, 25), 760},
  {IR(21922, 13), 716},
  {IR(21935, 22), 717},
  {IR(21957, 21), 718},
  {IR(22756, 24), 770},
  {IR(22905, 25), 777},
  {IR(22829, 26), 774},
  {IR(22930, 22), 778},
  {IR(22855, 23), 775},
  {IR(22682, 23), 766},
  {IR(22878, 27), 776},
  {IR(22801, 28), 773},
  {IR(23695, 24), 808},
  {IR(6579, 13), 748},
  {IR(22551, 30), 756},
  {IR(22373, 22), 749},
  {IR(22395, 24), 750},
  {IR(22633, 20), 764},
  {IR(22211, 10), 736},
  {IR(22324, 12), 744},
  {IR(21992, 15), 720},
  {IR(21978, 14), 719},
  {IR(22087, 4), 726},
  {IR(24030, 10), 825},
  {IR(24040, 9), 825},
  {IR(24091, 12), 828},
  {IR(24103, 11), 828},
  {IR(24049, 14), 826},
  {IR(24063, 13), 826},
  {IR(17029, 9), 818},
  {IR(23942, 10), 819},
  {IR(6060, 9), 817},
  {IR(23999, 16), 824},
  {IR(24015, 15), 824},
  {IR(5827, 7), 820},
  {IR(24122, 8), 830},
  {IR(23959, 7), 822},
  {IR(24076, 8), 827},
  {IR(24084, 7), 827},
  {IR(23966, 17), 823},
  {IR(23983, 16), 823},
  {IR(24114, 8), 829},
  {IR(23952, 7), 821},
  {IR(23899, 20), 815},
  {IR(23919, 23), 816},
  {IR(23892, 7), 814},
  {IR(24139, 12), 832},
  {IR(24130, 9), 831},
  {IR(24151, 12), 833},
  {IR(24219, 14), 838},
  {IR(24191, 14), 836},
  {IR(24205, 14), 837},
  {IR(24163, 14), 834},
  {IR(24177, 14), 835},
  {IR(10392, 22), 839},
  {IR(10414, 10), 840},
  {IR(24238, 4), 842},
  {IR(24233, 5), 841},
  {IR(24276, 14), 849},
  {IR(24290, 23), 849},
  {IR(24313, 13), 850},
  {IR(24326, 18), 850},
  {IR(24260, 11), 847},
  {IR(24344, 15), 851},
  {IR(24271, 5), 848},
  {IR(24256, 4), 846},
  {IR(16625, 5), 843},
  {IR(24249, 7), 845},
  {IR(24242, 7), 844},
  {IR(24359, 4), 852},
  {IR(24371, 4), 855},
  {IR(24367, 4), 854},
  {IR(24363, 4), 853},
  {IR(24407, 18), 858},
  {IR(24425, 18), 859},
  {IR(24375, 16), 856},
  {IR(24391, 16), 857},
  {IR(24466, 6), 864},
  {IR(24450, 11), 862},
  {IR(24443, 7), 861},
  {IR(16625, 5), 860},
  {IR(15433, 11), 865},
  {IR(15444, 13), 865},
  {IR(24461, 5), 863},
  {IR(24482, 6), 868},
  {IR(24493, 8), 870},
  {IR(24501, 10), 871},
  {IR(24519, 12), 873},
  {IR(24511, 8), 872},
  {IR(24531, 21), 874},
  {IR(24552, 20), 874},
  {IR(24477, 5), 867},
  {IR(24488, 5), 869},
  {IR(24472, 5), 866},
  {IR(24572, 14), 875},
  {IR(24586, 12), 876},
  {IR(24598, 13), 877},
  {IR(24611, 13), 878},
  {IR(24659, 16), 882},
  {IR(24645, 14), 881},
  {IR(24631, 14), 880},
  {IR(24771, 28), 885},
  {IR(24799, 27), 885},
  {IR(24716, 28), 884},
  {IR(24744, 27), 884},
  {IR(24675, 21), 883},
  {IR(24696, 20), 883},
  {IR(24624, 7), 879},
  {IR(24826, 10), 886},
  {IR(24836, 10), 887},
  {IR(24857, 15), 889},
  {IR(24846, 11), 888},
  {IR(24872, 5), 891},
  {IR(24886, 12), 894},
  {IR(24898, 13), 896},
  {IR(24881, 5), 893},
  {IR(24877, 4), 892},
  {IR(24911, 19), 899},
  {IR(24930, 22), 899},
  {IR(24952, 17), 900},
  {IR(24969, 20), 900},
  {IR(9660, 7), 898},
  {IR(24989, 16), 901},
  {IR(25005, 19), 901},
  {IR(16625, 5), 890},
  {IR(25077, 12), 905},
  {IR(18032, 7), 895},
  {IR(25089, 8), 906},
  {IR(17829, 7), 897},
  {IR(25055, 11), 903},
  {IR(25024, 14), 902},
  {IR(25038, 17), 902},
  {IR(25066, 11), 904},
  {IR(16522, 6), 921},
  {IR(25258, 10), 920},
  {IR(25193, 12), 915},
  {IR(25205, 12), 916},
  {IR(25182, 11), 914},
  {IR(25107, 11), 908},
  {IR(25097, 10), 907},
  {IR(25167, 15), 913},
  {IR(25279, 17), 923},
  {IR(25296, 16), 924},
  {IR(25429, 16), 931},
  {IR(25445, 16), 932},
  {IR(25128, 11), 910},
  {IR(25268, 11), 922},
  {IR(25352, 20), 927},
  {IR(25118, 10), 909},
  {IR(25153, 14), 912},
  {IR(25139, 14), 911},
  {IR(25372, 19), 928},
  {IR(25391, 19), 929},
  {IR(25410, 19), 930},
  {IR(25230, 14), 918},
  {IR(25244, 14), 919},
  {IR(25217, 13), 917},
  {IR(25312, 20), 925},
  {IR(25332, 20), 926},
  {IR(25463, 2), 934},
  {IR(25564, 5), 952},
  {IR(25485, 5), 940},
  {IR(25480, 5), 939},
  {IR(25522, 6), 946},
  {IR(25528, 13), 947},
  {IR(25490, 10), 941},
  {IR(25500, 10), 942},
  {IR(25461, 2), 933},
  {IR(25468, 3), 936},
  {IR(25465, 3), 935},
  {IR(25471, 4), 937},
  {IR(25475, 5), 938},
  {IR(25517, 5), 945},
  {IR(25513, 4), 944},
  {IR(25510, 3), 943},
  {IR(25558, 6), 951},
  {IR(25541, 5), 948},
  {IR(25552, 6), 950},
  {IR(25546, 6), 949},
  {IR(25569, 28), 953},
  {IR(25597, 27), 953},
  {IR(25624, 24), 954},
  {IR(25648, 23), 954},
  {IR(25671, 7), 955},
  {IR(25678, 11), 956},
  {IR(25689, 14), 957},
  {IR(25703, 12), 959},
  {IR(16625, 5), 958},
  {IR(25715, 13), 961},
  {IR(16625, 5), 960},
  {IR(25728, 7), 962},
  {IR(25735, 7), 963},
  {IR(25742, 12), 964},
  {IR(25754, 8), 965},
  {IR(25776, 12), 967},
  {IR(25828, 17), 970},
  {IR(25803, 25), 969},
  {IR(25788, 15), 968},
  {IR(25762, 14), 966},
  {IR(25968, 22), 983},
  {IR(25990, 21), 983},
  {IR(25863, 19), 974},
  {IR(25882, 17), 975},
  {IR(25852, 11), 973},
  {IR(20395, 25), 981},
  {IR(20420, 24), 981},
  {IR(25927, 18), 978},
  {IR(26011, 19), 985},
  {IR(26030, 18), 985},
  {IR(26173, 16), 989},
  {IR(26189, 15), 989},
  {IR(20444, 21), 982},
  {IR(20465, 20), 982},
  {IR(26048, 22), 986},
  {IR(26070, 21), 986},
  {IR(25913, 14), 977},
  {IR(26204, 27), 990},
  {IR(26231, 26), 990},
  {IR(25899, 14), 976},
  {IR(11197, 23), 991},
  {IR(26144, 15), 988},
  {IR(26159, 14), 988},
  {IR(16625, 5), 971},
  {IR(25955, 13), 980},
  {IR(25945, 10), 979},
  {IR(20485, 21), 984},
  {IR(20506, 20), 984},
  {IR(26091, 27), 987},
  {IR(26118, 26), 987},
  {IR(25845, 7), 972},
  {IR(26540, 27), 1005},
  {IR(26488, 26), 1003},
  {IR(26408, 23), 999},
  {IR(26362, 23), 997},
  {IR(26257, 29), 993},
  {IR(26454, 17), 1001},
  {IR(26567, 27), 1006},
  {IR(26514, 26), 1004},
  {IR(26431, 23), 1000},
  {IR(26385, 23), 998},
  {IR(26286, 29), 994},
  {IR(26471, 17), 1002},
  {IR(26315, 21), 995},
  {IR(26336, 26), 996},
  {IR(16625, 5), 992},
  {IR(26727, 20), 1014},
  {IR(26594, 8), 1009},
  {IR(26602, 21), 1011},
  {IR(26623, 24), 1011},
  {IR(26647, 19), 1012},
  {IR(26666, 22), 1012},
  {IR(26747, 17), 1015},
  {IR(26688, 18), 1013},
  {IR(26706, 21), 1013},
  {IR(16625, 5), 1007},
  {IR(25077, 12), 1010},
  {IR(17863, 9), 1008},
  {IR(26771, 8), 1017},
  {IR(26779, 7), 1018},
  {IR(26764, 7), 1016},
  {IR(26786, 7), 1019},
  {IR(26793, 10), 1019},
  {IR(26811, 8), 1021},
  {IR(26827, 15), 1023},
  {IR(26931, 20), 1029},
  {IR(26910, 21), 1028},
  {IR(26951, 12), 1030},
  {IR(26992, 14), 1032},
  {IR(27006, 17), 1032},
  {IR(27023, 12), 1033},
  {IR(27035, 15), 1033},
  {IR(16625, 5), 1020},
  {IR(26963, 13), 1031},
  {IR(26976, 16), 1031},
  {IR(26803, 8), 1020},
  {IR(26819, 8), 1022},
  {IR(26842, 23), 1024},
  {IR(26879, 15), 1026},
  {IR(26865, 14), 1025},
  {IR(17863, 9), 1034},
  {IR(26894, 16), 1027},
  {IR(27050, 10), 1035},
  {IR(27065, 4), 1037},
  {IR(27078, 8), 1039},
  {IR(27069, 9), 1038},
  {IR(27060, 5), 1036},
  {IR(27086, 25), 1040},
  {IR(27111, 28), 1040},
  {IR(27152, 4), 1043},
  {IR(27185, 9), 1047},
  {IR(27194, 13), 1048},
  {IR(27165, 8), 1045},
  {IR(27173, 12), 1046},
  {IR(27156, 9), 1044},
  {IR(27139, 4), 1041},
  {IR(27143, 9), 1042},
  {IR(16625, 5), 1049},
  {IR(27207, 25), 1050},
  {IR(27232, 23), 1051},
  {IR(27324, 27), 1057},
  {IR(27351, 28), 1058},
  {IR(27393, 16), 1060},
  {IR(27379, 14), 1059},
  {IR(27493, 30), 1063},
  {IR(27463, 30), 1063},
  {IR(27265, 12), 1054},
  {IR(17377, 8), 1052},
  {IR(27255, 10), 1053},
  {IR(27450, 13), 1062},
  {IR(27426, 24), 1061},
  {IR(27300, 24), 1056},
  {IR(27409, 17), 1061},
  {IR(27277, 23), 1055},
  {IR(27564, 37), 1065},
  {IR(27523, 41), 1064},
  {IR(27679, 42), 1068},
  {IR(27601, 37), 1066},
  {IR(27638, 41), 1067},
  {IR(27721, 33), 1069},
  {IR(27754, 33), 1070},
  {IR(27799, 6), 1073},
  {IR(27787, 12), 1072},
  {IR(16625, 5), 1071},
  {IR(27805, 7), 1074},
  {IR(27812, 15), 1075},
  {IR(27835, 7), 1077},
  {IR(27827, 8), 1076},
  {IR(27900, 13), 1086},
  {IR(27942, 4), 1092},
  {IR(27980, 9), 1097},
  {IR(27913, 5), 1087},
  {IR(28042, 5), 1106},
  {IR(28113, 6), 1116},
  {IR(27866, 5), 1081},
  {IR(28020, 5), 1102},
  {IR(28077, 6), 1111},
  {IR(28130, 5), 1119},
  {IR(28124, 6), 1118},
  {IR(27946, 3), 1093},
  {IR(27989, 8), 1098},
  {IR(28047, 4), 1107},
  {IR(28119, 5), 1117},
  {IR(27933, 5), 1090},
  {IR(27961, 10), 1095},
  {IR(27894, 6), 1085},
  {IR(28031, 6), 1104},
  {IR(28100, 7), 1114},
  {IR(27888, 6), 1084},
  {IR(28025, 6), 1103},
  {IR(28093, 7), 1113},
  {IR(27938, 4), 1091},
  {IR(27971, 9), 1096},
  {IR(28037, 5), 1105},
  {IR(28107, 6), 1115},
  {IR(27925, 8), 1089},
  {IR(28083, 10), 1112},
  {IR(27918, 7), 1088},
  {IR(27949, 12), 1094},
  {IR(27858, 8), 1080},
  {IR(28005, 8), 1100},
  {IR(28060, 9), 1109},
  {IR(27850, 8), 1079},
  {IR(27997, 8), 1099},
  {IR(28051, 9), 1108},
  {IR(27871, 6), 1082},
  {IR(27877, 11), 1083},
  {IR(28013, 7), 1101},
  {IR(28069, 8), 1110},
  {IR(27842, 8), 1078},
  {IR(28135, 12), 1120},
  {IR(28147, 7), 1121},
  {IR(28173, 11), 1124},
  {IR(28184, 12), 1125},
  {IR(28196, 15), 1125},
  {IR(28211, 14), 1126},
  {IR(28164, 9), 1123},
  {IR(28154, 10), 1122},
  {IR(28233, 12), 1129},
  {IR(28225, 8), 1128},
  {IR(16625, 5), 1127},
  {IR(28245, 29), 1130},
  {IR(16506, 8), 1132},
  {IR(16514, 8), 1133},
  {IR(16522, 6), 1134},
  {IR(16528, 7), 1135},
  {IR(16535, 11), 1136},
  {IR(16546, 9), 1137},
  {IR(16555, 13), 1138},
  {IR(16494, 12), 1131},
  {IR(16568, 6), 1139},
  {IR(16574, 10), 1140},
  {IR(16584, 6), 1141},
  {IR(16605, 20), 1143},
  {IR(16590, 15), 1142},
  {IR(16714, 15), 1151},
  {IR(16729, 13), 1152},
  {IR(16702, 12), 1150},
  {IR(16792, 21), 1156},
  {IR(16685, 17), 1149},
  {IR(16869, 16), 1160},
  {IR(16673, 12), 1148},
  {IR(16853, 16), 1159},
  {IR(16646, 14), 1146},
  {IR(16630, 16), 1145},
  {IR(16660, 13), 1147},
  {IR(16813, 20), 1157},
  {IR(16757, 18), 1154},
  {IR(16742, 15), 1153},
  {IR(16833, 20), 1158},
  {IR(16775, 17), 1155},
  {IR(16905, 24), 1162},
  {IR(16885, 20), 1161},
  {IR(28274, 26), 1163},
  {IR(16625, 5), 1144},
  {IR(16990, 9), 1168},
  {IR(17022, 7), 1172},
  {IR(16929, 6), 1164},
  {IR(17029, 9), 1173},
  {IR(16940, 6), 1166},
  {IR(16935, 5), 1165},
  {IR(16979, 11), 1167},
  {IR(17011, 11), 1171},
  {IR(16999, 5), 1169},
  {IR(17004, 7), 1170},
  {IR(17074, 11), 1177},
  {IR(17038, 10), 1174},
  {IR(17061, 13), 1176},
  {IR(17048, 13), 1175},
  {IR(28392, 9), 1193},
  {IR(28335, 15), 1184},
  {IR(28300, 5), 1179},
  {IR(28305, 5), 1180},
  {IR(28355, 7), 1186},
  {IR(28330, 5), 1183},
  {IR(28362, 5), 1187},
  {IR(28310, 9), 1181},
  {IR(28319, 11), 1182},
  {IR(28387, 5), 1192},
  {IR(28382, 5), 1191},
  {IR(28350, 5), 1185},
  {IR(28372, 6), 1189},
  {IR(27842, 8), 1178},
  {IR(28367, 5), 1188},
  {IR(28378, 4), 1190},
  {IR(28483, 14), 1204},
  {IR(28565, 16), 1210},
  {IR(28581, 15), 1210},
  {IR(28901, 17), 1220},
  {IR(28430, 15), 1199},
  {IR(28918, 17), 1221},
  {IR(28935, 16), 1221},
  {IR(28453, 9), 1201},
  {IR(28462, 8), 1202},
  {IR(28670, 16), 1213},
  {IR(28686, 15), 1213},
  {IR(28879, 22), 1219},
  {IR(28834, 21), 1217},
  {IR(28951, 15), 1222},
  {IR(28966, 14), 1222},
  {IR(28497, 6), 1205},
  {IR(28596, 24), 1211},
  {IR(28620, 23), 1211},
  {IR(28701, 22), 1214},
  {IR(28723, 21), 1214},
  {IR(28417, 6), 1195},
  {IR(28549, 16), 1209},
  {IR(28423, 7), 1197},
  {IR(28787, 22), 1216},
  {IR(28809, 25), 1216},
  {IR(28445, 8), 1200},
  {IR(28470, 13), 1203},
  {IR(28643, 14), 1212},
  {IR(28657, 13), 1212},
  {IR(28744, 22), 1215},
  {IR(28766, 21), 1215},
  {IR(28503, 14), 1206},
  {IR(28855, 24), 1218},
  {IR(28530, 19), 1208},
  {IR(28517, 13), 1207},
  {IR(17914, 8), 1196},
  {IR(28401, 16), 1194},
  {IR(28154, 10), 1198},
  {IR(25788, 15), 1226},
  {IR(25762, 14), 1223},
  {IR(28998, 15), 1225},
  {IR(28980, 18), 1224},
  {IR(29024, 11), 1229},
  {IR(29035, 17), 1230},
  {IR(16625, 5), 1227},
  {IR(29013, 11), 1228},
  {IR(27787, 12), 1233},
  {IR(17872, 9), 1232},
  {IR(27805, 7), 1234},
  {IR(27812, 15), 1235},
  {IR(29052, 10), 1231},
  {IR(29105, 24), 1239},
  {IR(29129, 22), 1240},
  {IR(29151, 21), 1241},
  {IR(29062, 8), 1236},
  {IR(29070, 15), 1237},
  {IR(29085, 20), 1238},
}};

IndexRange OperandNameRangeForKind(spv_operand_type_t type) {
  switch(type) {
    case SPV_OPERAND_TYPE_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_ADDRESSING_MODEL: return IR(3, 5);
    case SPV_OPERAND_TYPE_BUILT_IN: return IR(8, 150);
    case SPV_OPERAND_TYPE_CAPABILITY: return IR(158, 353);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(511, 8);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(519, 3);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(522, 2);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS: return IR(524, 19);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION: return IR(543, 10);
    case SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(553, 4);
    case SPV_OPERAND_TYPE_COMPONENT_TYPE: return IR(557, 15);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_LAYOUT: return IR(572, 4);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_OPERANDS: return IR(576, 6);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE: return IR(582, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_USE: return IR(585, 3);
    case SPV_OPERAND_TYPE_COOPERATIVE_VECTOR_MATRIX_LAYOUT: return IR(588, 4);
    case SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(592, 8);
    case SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE: return IR(600, 3);
    case SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS: return IR(603, 16);
    case SPV_OPERAND_TYPE_DEBUG_OPERATION: return IR(619, 9);
    case SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER: return IR(628, 3);
    case SPV_OPERAND_TYPE_DECORATION: return IR(631, 199);
    case SPV_OPERAND_TYPE_DIMENSIONALITY: return IR(830, 8);
    case SPV_OPERAND_TYPE_EXECUTION_MODE: return IR(838, 104);
    case SPV_OPERAND_TYPE_EXECUTION_MODEL: return IR(942, 23);
    case SPV_OPERAND_TYPE_FPDENORM_MODE: return IR(965, 2);
    case SPV_OPERAND_TYPE_FPENCODING: return IR(967, 8);
    case SPV_OPERAND_TYPE_FPOPERATION_MODE: return IR(975, 2);
    case SPV_OPERAND_TYPE_FP_FAST_MATH_MODE: return IR(977, 11);
    case SPV_OPERAND_TYPE_FP_ROUNDING_MODE: return IR(988, 4);
    case SPV_OPERAND_TYPE_FRAGMENT_SHADING_RATE: return IR(992, 4);
    case SPV_OPERAND_TYPE_FUNCTION_CONTROL: return IR(996, 7);
    case SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE: return IR(1003, 10);
    case SPV_OPERAND_TYPE_GATHER_MODES: return IR(1013, 4);
    case SPV_OPERAND_TYPE_GROUP_OPERATION: return IR(1017, 10);
    case SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER: return IR(1027, 4);
    case SPV_OPERAND_TYPE_IMAGE: return IR(1031, 21);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE: return IR(1052, 26);
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER: return IR(1078, 20);
    case SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER: return IR(1098, 4);
    case SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS: return IR(1102, 3);
    case SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO: return IR(1105, 2);
    case SPV_OPERAND_TYPE_KERNEL_PROPERTY_FLAGS: return IR(1107, 2);
    case SPV_OPERAND_TYPE_LINKAGE_TYPE: return IR(1109, 4);
    case SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL: return IR(1113, 5);
    case SPV_OPERAND_TYPE_LOOP_CONTROL: return IR(1118, 31);
    case SPV_OPERAND_TYPE_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(1149, 15);
    case SPV_OPERAND_TYPE_MEMORY_ACCESS: return IR(1164, 12);
    case SPV_OPERAND_TYPE_MEMORY_MODEL: return IR(1176, 5);
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID: return IR(1181, 19);
    case SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS: return IR(1200, 1);
    case SPV_OPERAND_TYPE_OVERFLOW_MODES: return IR(1201, 4);
    case SPV_OPERAND_TYPE_PACKED_VECTOR_FORMAT: return IR(1205, 2);
    case SPV_OPERAND_TYPE_QUANTIZATION_MODES: return IR(1207, 8);
    case SPV_OPERAND_TYPE_RAW_ACCESS_CHAIN_OPERANDS: return IR(1215, 3);
    case SPV_OPERAND_TYPE_RAY_FLAGS: return IR(1218, 14);
    case SPV_OPERAND_TYPE_RAY_QUERY_CANDIDATE_INTERSECTION_TYPE: return IR(1232, 2);
    case SPV_OPERAND_TYPE_RAY_QUERY_COMMITTED_INTERSECTION_TYPE: return IR(1234, 3);
    case SPV_OPERAND_TYPE_RAY_QUERY_INTERSECTION: return IR(1237, 2);
    case SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE: return IR(1239, 5);
    case SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE: return IR(1244, 2);
    case SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT: return IR(1246, 42);
    case SPV_OPERAND_TYPE_SCOPE_ID: return IR(1288, 8);
    case SPV_OPERAND_TYPE_SELECTION_CONTROL: return IR(1296, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_BUILD_IDENTIFIER_FLAGS: return IR(1299, 1);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING: return IR(1300, 8);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_COMPOSITE_TYPE: return IR(1308, 3);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_IMPORTED_ENTITY: return IR(1311, 2);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_INFO_FLAGS: return IR(1313, 20);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_OPERATION: return IR(1333, 10);
    case SPV_OPERAND_TYPE_SHDEBUG100_DEBUG_TYPE_QUALIFIER: return IR(1343, 4);
    case SPV_OPERAND_TYPE_SOURCE_LANGUAGE: return IR(1347, 16);
    case SPV_OPERAND_TYPE_STORAGE_CLASS: return IR(1363, 38);
    case SPV_OPERAND_TYPE_STORE_CACHE_CONTROL: return IR(1401, 4);
    case SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS: return IR(1405, 4);
    case SPV_OPERAND_TYPE_TENSOR_CLAMP_MODE: return IR(1409, 5);
    case SPV_OPERAND_TYPE_TENSOR_OPERANDS: return IR(1414, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_IMAGE: return IR(1031, 21);
    case SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER: return IR(0, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS: return IR(1164, 12);
    case SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT: return IR(1205, 2);
    case SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS: return IR(576, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS: return IR(1149, 15);
    case SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS: return IR(1215, 3);
    case SPV_OPERAND_TYPE_OPTIONAL_FPENCODING: return IR(967, 8);
    case SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS: return IR(1414, 6);
    case SPV_OPERAND_TYPE_OPTIONAL_CAPABILITY: return IR(158, 353);
    default: break;
  }
  return IR(0,0);
}













static const std::array<InstructionDesc, 875> kInstructionDesc{{
  {spv::Op::OpNop, false, false, IR(0, 0), IR(29172, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpUndef, true, true, IR(39, 2), IR(29228, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpSourceContinued, false, false, IR(11, 1), IR(29234, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpSource, false, false, IR(41, 4), IR(29353, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpSourceExtension, false, false, IR(11, 1), IR(29360, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpName, false, false, IR(45, 2), IR(29376, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpMemberName, false, false, IR(47, 3), IR(29381, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpString, true, false, IR(50, 2), IR(29392, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpLine, false, false, IR(52, 3), IR(29399, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpExtension, false, false, IR(11, 1), IR(29404, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpExtInstImport, true, false, IR(50, 2), IR(29414, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpExtInst, true, true, IR(55, 4), IR(29474, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpMemoryModel, false, false, IR(59, 2), IR(29546, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpEntryPoint, false, false, IR(61, 4), IR(29620, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpExecutionMode, false, false, IR(65, 2), IR(29663, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpCapability, false, false, IR(67, 1), IR(29705, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpTypeVoid, true, false, IR(68, 1), IR(29716, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeBool, true, false, IR(68, 1), IR(29725, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeInt, true, false, IR(69, 3), IR(29734, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeFloat, true, false, IR(72, 3), IR(29779, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeVector, true, false, IR(75, 3), IR(29789, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeMatrix, true, false, IR(75, 3), IR(29800, 11), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeImage, true, false, IR(78, 9), IR(29924, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeSampler, true, false, IR(68, 1), IR(29934, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeSampledImage, true, false, IR(87, 2), IR(29946, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeArray, true, false, IR(89, 3), IR(29963, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeRuntimeArray, true, false, IR(87, 2), IR(29973, 17), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeStruct, true, false, IR(92, 2), IR(29990, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeOpaque, true, false, IR(50, 2), IR(30001, 11), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypePointer, true, false, IR(94, 3), IR(30043, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeFunction, true, false, IR(97, 3), IR(30055, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeEvent, true, false, IR(68, 1), IR(30068, 10), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeDeviceEvent, true, false, IR(68, 1), IR(30078, 16), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeReserveId, true, false, IR(68, 1), IR(30094, 14), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeQueue, true, false, IR(68, 1), IR(30108, 10), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypePipe, true, false, IR(100, 2), IR(30118, 9), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeForwardPointer, false, false, IR(102, 2), IR(30127, 19), IR(0, 0), IR(220, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpConstantTrue, true, true, IR(39, 2), IR(30146, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantFalse, true, true, IR(39, 2), IR(30159, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstant, true, true, IR(104, 3), IR(17872, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantComposite, true, true, IR(107, 3), IR(30211, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantSampler, true, true, IR(110, 5), IR(30307, 16), IR(0, 0), IR(222, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantNull, true, true, IR(39, 2), IR(30323, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantTrue, true, true, IR(39, 2), IR(30336, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantFalse, true, true, IR(39, 2), IR(30353, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstant, true, true, IR(104, 3), IR(30371, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantComposite, true, true, IR(107, 3), IR(30384, 22), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantOp, true, true, IR(115, 3), IR(30447, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpFunction, true, true, IR(118, 4), IR(28453, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpFunctionParameter, true, true, IR(39, 2), IR(30496, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpFunctionEnd, false, false, IR(0, 0), IR(30514, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpFunctionCall, true, true, IR(122, 4), IR(30526, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpVariable, true, true, IR(126, 4), IR(30539, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpImageTexelPointer, true, true, IR(130, 5), IR(30548, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpLoad, true, true, IR(135, 4), IR(30606, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpStore, false, false, IR(139, 3), IR(30611, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCopyMemory, false, false, IR(142, 4), IR(30617, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCopyMemorySized, false, false, IR(146, 5), IR(30628, 16), IR(0, 0), IR(223, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpAccessChain, true, true, IR(122, 4), IR(30644, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpInBoundsAccessChain, true, true, IR(122, 4), IR(30656, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPtrAccessChain, true, true, IR(151, 5), IR(30676, 15), IR(0, 0), IR(225, 4), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpArrayLength, true, true, IR(156, 4), IR(30691, 12), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpGenericPtrMemSemantics, true, true, IR(160, 3), IR(30703, 23), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpInBoundsPtrAccessChain, true, true, IR(151, 5), IR(30726, 23), IR(0, 0), IR(1, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpDecorate, false, false, IR(163, 2), IR(30777, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpMemberDecorate, false, false, IR(165, 3), IR(30786, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpDecorationGroup, true, false, IR(68, 1), IR(30801, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpGroupDecorate, false, false, IR(168, 2), IR(30817, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpGroupMemberDecorate, false, false, IR(170, 2), IR(30876, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpVectorExtractDynamic, true, true, IR(172, 4), IR(30896, 21), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpVectorInsertDynamic, true, true, IR(130, 5), IR(30917, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpVectorShuffle, true, true, IR(176, 5), IR(30937, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeConstruct, true, true, IR(107, 3), IR(30951, 19), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeExtract, true, true, IR(181, 4), IR(30970, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeInsert, true, true, IR(176, 5), IR(30987, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCopyObject, true, true, IR(160, 3), IR(31003, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpTranspose, true, true, IR(160, 3), IR(31014, 10), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpSampledImage, true, true, IR(172, 4), IR(31024, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleImplicitLod, true, true, IR(185, 5), IR(31069, 23), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleExplicitLod, true, true, IR(190, 5), IR(31115, 23), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleDrefImplicitLod, true, true, IR(195, 6), IR(31138, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleDrefExplicitLod, true, true, IR(201, 6), IR(31165, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjImplicitLod, true, true, IR(185, 5), IR(31192, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjExplicitLod, true, true, IR(190, 5), IR(31219, 27), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjDrefImplicitLod, true, true, IR(195, 6), IR(31246, 31), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSampleProjDrefExplicitLod, true, true, IR(201, 6), IR(31277, 31), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageFetch, true, true, IR(185, 5), IR(31308, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageGather, true, true, IR(195, 6), IR(31319, 12), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageDrefGather, true, true, IR(195, 6), IR(31331, 16), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageRead, true, true, IR(185, 5), IR(31347, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageWrite, false, false, IR(207, 4), IR(31357, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImage, true, true, IR(160, 3), IR(28497, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryFormat, true, true, IR(160, 3), IR(31368, 17), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryOrder, true, true, IR(160, 3), IR(31385, 16), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQuerySizeLod, true, true, IR(172, 4), IR(31401, 18), IR(0, 0), IR(229, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQuerySize, true, true, IR(160, 3), IR(31419, 15), IR(0, 0), IR(229, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryLod, true, true, IR(172, 4), IR(31434, 14), IR(0, 0), IR(231, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQueryLevels, true, true, IR(160, 3), IR(31448, 17), IR(0, 0), IR(229, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageQuerySamples, true, true, IR(160, 3), IR(31465, 18), IR(0, 0), IR(229, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpConvertFToU, true, true, IR(160, 3), IR(31483, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertFToS, true, true, IR(160, 3), IR(31495, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertSToF, true, true, IR(160, 3), IR(31507, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertUToF, true, true, IR(160, 3), IR(31519, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpUConvert, true, true, IR(160, 3), IR(31531, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSConvert, true, true, IR(160, 3), IR(31540, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpFConvert, true, true, IR(160, 3), IR(31549, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpQuantizeToF16, true, true, IR(160, 3), IR(31558, 14), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertPtrToU, true, true, IR(160, 3), IR(31572, 14), IR(0, 0), IR(220, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSatConvertSToU, true, true, IR(160, 3), IR(31586, 15), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSatConvertUToS, true, true, IR(160, 3), IR(31601, 15), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertUToPtr, true, true, IR(160, 3), IR(31616, 14), IR(0, 0), IR(220, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpPtrCastToGeneric, true, true, IR(160, 3), IR(31630, 17), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpGenericCastToPtr, true, true, IR(160, 3), IR(31647, 17), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpGenericCastToPtrExplicit, true, true, IR(211, 4), IR(31664, 25), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpBitcast, true, true, IR(160, 3), IR(31689, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpSNegate, true, true, IR(160, 3), IR(31697, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFNegate, true, true, IR(160, 3), IR(31705, 8), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpIAdd, true, true, IR(172, 4), IR(31713, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFAdd, true, true, IR(172, 4), IR(31718, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpISub, true, true, IR(172, 4), IR(31723, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFSub, true, true, IR(172, 4), IR(31728, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpIMul, true, true, IR(172, 4), IR(31733, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFMul, true, true, IR(172, 4), IR(31738, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUDiv, true, true, IR(172, 4), IR(31743, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSDiv, true, true, IR(172, 4), IR(31748, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFDiv, true, true, IR(172, 4), IR(31753, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUMod, true, true, IR(172, 4), IR(31758, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSRem, true, true, IR(172, 4), IR(31763, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSMod, true, true, IR(172, 4), IR(31768, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFRem, true, true, IR(172, 4), IR(31773, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpFMod, true, true, IR(172, 4), IR(31778, 5), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpVectorTimesScalar, true, true, IR(172, 4), IR(31783, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpMatrixTimesScalar, true, true, IR(172, 4), IR(31801, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpVectorTimesMatrix, true, true, IR(172, 4), IR(31819, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpMatrixTimesVector, true, true, IR(172, 4), IR(31837, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpMatrixTimesMatrix, true, true, IR(172, 4), IR(31855, 18), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpOuterProduct, true, true, IR(172, 4), IR(31873, 13), IR(0, 0), IR(67, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpDot, true, true, IR(172, 4), IR(31886, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpIAddCarry, true, true, IR(172, 4), IR(31890, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpISubBorrow, true, true, IR(172, 4), IR(31900, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUMulExtended, true, true, IR(172, 4), IR(31911, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSMulExtended, true, true, IR(172, 4), IR(31924, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpAny, true, true, IR(160, 3), IR(31937, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpAll, true, true, IR(160, 3), IR(31941, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsNan, true, true, IR(160, 3), IR(31945, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsInf, true, true, IR(160, 3), IR(31951, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsFinite, true, true, IR(160, 3), IR(31957, 9), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIsNormal, true, true, IR(160, 3), IR(31966, 9), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSignBitSet, true, true, IR(160, 3), IR(31975, 11), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLessOrGreater, true, true, IR(172, 4), IR(31986, 14), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), SPV_SPIRV_VERSION_WORD(1,5), PrintingClass::kRelational_and_Logical},
  {spv::Op::OpOrdered, true, true, IR(172, 4), IR(32000, 8), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpUnordered, true, true, IR(172, 4), IR(32008, 10), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalEqual, true, true, IR(172, 4), IR(32018, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalNotEqual, true, true, IR(172, 4), IR(32031, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalOr, true, true, IR(172, 4), IR(32047, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalAnd, true, true, IR(172, 4), IR(32057, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpLogicalNot, true, true, IR(160, 3), IR(32068, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSelect, true, true, IR(130, 5), IR(32079, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpIEqual, true, true, IR(172, 4), IR(32086, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpINotEqual, true, true, IR(172, 4), IR(32093, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpUGreaterThan, true, true, IR(172, 4), IR(32103, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSGreaterThan, true, true, IR(172, 4), IR(32116, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpUGreaterThanEqual, true, true, IR(172, 4), IR(32129, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSGreaterThanEqual, true, true, IR(172, 4), IR(32147, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpULessThan, true, true, IR(172, 4), IR(32165, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSLessThan, true, true, IR(172, 4), IR(32175, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpULessThanEqual, true, true, IR(172, 4), IR(32185, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpSLessThanEqual, true, true, IR(172, 4), IR(32200, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdEqual, true, true, IR(172, 4), IR(32215, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordEqual, true, true, IR(172, 4), IR(32225, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdNotEqual, true, true, IR(172, 4), IR(32237, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordNotEqual, true, true, IR(172, 4), IR(32250, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdLessThan, true, true, IR(172, 4), IR(32265, 13), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordLessThan, true, true, IR(172, 4), IR(32278, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdGreaterThan, true, true, IR(172, 4), IR(32293, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordGreaterThan, true, true, IR(172, 4), IR(32309, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdLessThanEqual, true, true, IR(172, 4), IR(32327, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordLessThanEqual, true, true, IR(172, 4), IR(32345, 20), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFOrdGreaterThanEqual, true, true, IR(172, 4), IR(32365, 21), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpFUnordGreaterThanEqual, true, true, IR(172, 4), IR(32386, 23), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kRelational_and_Logical},
  {spv::Op::OpShiftRightLogical, true, true, IR(172, 4), IR(32409, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpShiftRightArithmetic, true, true, IR(172, 4), IR(32427, 21), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpShiftLeftLogical, true, true, IR(172, 4), IR(32448, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitwiseOr, true, true, IR(172, 4), IR(32465, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitwiseXor, true, true, IR(172, 4), IR(32475, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitwiseAnd, true, true, IR(172, 4), IR(32486, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpNot, true, true, IR(160, 3), IR(32497, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitFieldInsert, true, true, IR(215, 6), IR(32501, 15), IR(0, 0), IR(232, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitFieldSExtract, true, true, IR(130, 5), IR(32516, 17), IR(0, 0), IR(232, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitFieldUExtract, true, true, IR(130, 5), IR(32533, 17), IR(0, 0), IR(232, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitReverse, true, true, IR(160, 3), IR(32550, 11), IR(0, 0), IR(232, 2), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpBitCount, true, true, IR(160, 3), IR(32561, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpDPdx, true, true, IR(160, 3), IR(32570, 5), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdy, true, true, IR(160, 3), IR(32575, 5), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpFwidth, true, true, IR(160, 3), IR(32580, 7), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdxFine, true, true, IR(160, 3), IR(32587, 9), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdyFine, true, true, IR(160, 3), IR(32596, 9), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpFwidthFine, true, true, IR(160, 3), IR(32605, 11), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdxCoarse, true, true, IR(160, 3), IR(32616, 11), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpDPdyCoarse, true, true, IR(160, 3), IR(32627, 11), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpFwidthCoarse, true, true, IR(160, 3), IR(32638, 13), IR(0, 0), IR(234, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDerivative},
  {spv::Op::OpEmitVertex, false, false, IR(0, 0), IR(32651, 11), IR(0, 0), IR(70, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpEndPrimitive, false, false, IR(0, 0), IR(32662, 13), IR(0, 0), IR(70, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpEmitStreamVertex, false, false, IR(10, 1), IR(32675, 17), IR(0, 0), IR(123, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpEndStreamPrimitive, false, false, IR(10, 1), IR(32692, 19), IR(0, 0), IR(123, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPrimitive},
  {spv::Op::OpControlBarrier, false, false, IR(221, 3), IR(32748, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpMemoryBarrier, false, false, IR(224, 2), IR(32763, 14), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpAtomicLoad, true, true, IR(226, 5), IR(32777, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicStore, false, false, IR(231, 4), IR(32788, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicExchange, true, true, IR(235, 6), IR(32800, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicCompareExchange, true, true, IR(241, 8), IR(32815, 22), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicCompareExchangeWeak, true, true, IR(241, 8), IR(32837, 26), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), SPV_SPIRV_VERSION_WORD(1,3), PrintingClass::kAtomic},
  {spv::Op::OpAtomicIIncrement, true, true, IR(226, 5), IR(32863, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicIDecrement, true, true, IR(226, 5), IR(32880, 17), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicIAdd, true, true, IR(235, 6), IR(32897, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicISub, true, true, IR(235, 6), IR(32908, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicSMin, true, true, IR(235, 6), IR(32919, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicUMin, true, true, IR(235, 6), IR(32930, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicSMax, true, true, IR(235, 6), IR(32941, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicUMax, true, true, IR(235, 6), IR(32952, 11), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicAnd, true, true, IR(235, 6), IR(32963, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicOr, true, true, IR(235, 6), IR(32973, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicXor, true, true, IR(235, 6), IR(32982, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpPhi, true, true, IR(107, 3), IR(32992, 4), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLoopMerge, false, false, IR(249, 3), IR(33026, 10), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpSelectionMerge, false, false, IR(252, 2), IR(33071, 15), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLabel, true, false, IR(68, 1), IR(33086, 6), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpBranch, false, false, IR(10, 1), IR(33092, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpBranchConditional, false, false, IR(254, 4), IR(33099, 18), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpSwitch, false, false, IR(258, 3), IR(33162, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpKill, false, false, IR(0, 0), IR(33169, 5), IR(0, 0), IR(3, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpReturn, false, false, IR(0, 0), IR(33174, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpReturnValue, false, false, IR(10, 1), IR(33181, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpUnreachable, false, false, IR(0, 0), IR(33193, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLifetimeStart, false, false, IR(261, 2), IR(33205, 14), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpLifetimeStop, false, false, IR(261, 2), IR(33219, 13), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpGroupAsyncCopy, true, true, IR(263, 8), IR(33232, 15), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupWaitEvents, false, false, IR(271, 3), IR(33247, 16), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupAll, true, true, IR(274, 4), IR(33263, 9), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupAny, true, true, IR(274, 4), IR(33272, 9), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBroadcast, true, true, IR(278, 5), IR(33281, 15), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupIAdd, true, true, IR(283, 5), IR(33329, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFAdd, true, true, IR(283, 5), IR(33339, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMin, true, true, IR(283, 5), IR(33349, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMin, true, true, IR(283, 5), IR(33359, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMin, true, true, IR(283, 5), IR(33369, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMax, true, true, IR(283, 5), IR(33379, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMax, true, true, IR(283, 5), IR(33389, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMax, true, true, IR(283, 5), IR(33399, 10), IR(0, 0), IR(235, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpReadPipe, true, true, IR(215, 6), IR(33409, 9), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpWritePipe, true, true, IR(215, 6), IR(33418, 10), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReservedReadPipe, true, true, IR(288, 8), IR(33428, 17), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReservedWritePipe, true, true, IR(288, 8), IR(33445, 18), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReserveReadPipePackets, true, true, IR(215, 6), IR(33463, 23), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpReserveWritePipePackets, true, true, IR(215, 6), IR(33486, 24), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpCommitReadPipe, false, false, IR(296, 4), IR(33510, 15), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpCommitWritePipe, false, false, IR(296, 4), IR(33525, 16), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpIsValidReserveId, true, true, IR(160, 3), IR(33541, 17), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGetNumPipePackets, true, true, IR(130, 5), IR(33558, 18), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGetMaxPipePackets, true, true, IR(130, 5), IR(33576, 18), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupReserveReadPipePackets, true, true, IR(300, 7), IR(33594, 28), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupReserveWritePipePackets, true, true, IR(300, 7), IR(33622, 29), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupCommitReadPipe, false, false, IR(307, 5), IR(33651, 20), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGroupCommitWritePipe, false, false, IR(307, 5), IR(33671, 21), IR(0, 0), IR(76, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpEnqueueMarker, true, true, IR(215, 6), IR(33692, 14), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpEnqueueKernel, true, true, IR(312, 13), IR(33706, 14), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelNDrangeSubGroupCount, true, true, IR(325, 7), IR(33720, 30), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelNDrangeMaxSubGroupSize, true, true, IR(325, 7), IR(33750, 32), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelWorkGroupSize, true, true, IR(215, 6), IR(33782, 23), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelPreferredWorkGroupSizeMultiple, true, true, IR(215, 6), IR(33805, 40), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpRetainEvent, false, false, IR(10, 1), IR(33845, 12), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpReleaseEvent, false, false, IR(10, 1), IR(33857, 13), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpCreateUserEvent, true, true, IR(39, 2), IR(33870, 16), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpIsValidEvent, true, true, IR(160, 3), IR(33886, 13), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpSetUserEventStatus, false, false, IR(36, 2), IR(33899, 19), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpCaptureEventProfilingInfo, false, false, IR(33, 3), IR(33918, 26), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetDefaultQueue, true, true, IR(39, 2), IR(33944, 16), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpBuildNDRange, true, true, IR(130, 5), IR(33960, 13), IR(0, 0), IR(75, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpImageSparseSampleImplicitLod, true, true, IR(185, 5), IR(33973, 29), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleExplicitLod, true, true, IR(190, 5), IR(34002, 29), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleDrefImplicitLod, true, true, IR(195, 6), IR(34031, 33), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleDrefExplicitLod, true, true, IR(201, 6), IR(34064, 33), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjImplicitLod, true, true, IR(185, 5), IR(34097, 33), IR(0, 0), IR(236, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjExplicitLod, true, true, IR(190, 5), IR(34130, 33), IR(0, 0), IR(236, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjDrefImplicitLod, true, true, IR(195, 6), IR(34163, 37), IR(0, 0), IR(236, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseSampleProjDrefExplicitLod, true, true, IR(201, 6), IR(34200, 37), IR(0, 0), IR(236, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseFetch, true, true, IR(185, 5), IR(34237, 17), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseGather, true, true, IR(195, 6), IR(34254, 18), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseDrefGather, true, true, IR(195, 6), IR(34272, 22), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageSparseTexelsResident, true, true, IR(160, 3), IR(34294, 26), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpNoLine, false, false, IR(0, 0), IR(34320, 7), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpAtomicFlagTestAndSet, true, true, IR(226, 5), IR(34327, 21), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicFlagClear, false, false, IR(332, 3), IR(34348, 16), IR(0, 0), IR(0, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpImageSparseRead, true, true, IR(185, 5), IR(34364, 16), IR(0, 0), IR(236, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,0), 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpSizeOf, true, true, IR(160, 3), IR(34380, 7), IR(0, 0), IR(1, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpTypePipeStorage, true, false, IR(68, 1), IR(34387, 16), IR(0, 0), IR(237, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpConstantPipeStorage, true, true, IR(335, 5), IR(34403, 20), IR(0, 0), IR(237, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpCreatePipeFromPipeStorage, true, true, IR(160, 3), IR(34423, 26), IR(0, 0), IR(237, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kPipe},
  {spv::Op::OpGetKernelLocalSizeForSubgroupCount, true, true, IR(325, 7), IR(34449, 35), IR(0, 0), IR(164, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpGetKernelMaxNumSubgroups, true, true, IR(215, 6), IR(34484, 25), IR(0, 0), IR(164, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kDevice_Side_Enqueue},
  {spv::Op::OpTypeNamedBarrier, true, false, IR(68, 1), IR(34509, 17), IR(0, 0), IR(238, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpNamedBarrierInitialize, true, true, IR(160, 3), IR(34526, 23), IR(0, 0), IR(238, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpMemoryNamedBarrier, false, false, IR(332, 3), IR(34549, 19), IR(0, 0), IR(238, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpModuleProcessed, false, false, IR(11, 1), IR(34568, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,1), 0xffffffffu, PrintingClass::kDebug},
  {spv::Op::OpExecutionModeId, false, false, IR(65, 2), IR(34584, 16), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,2), 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpDecorateId, false, false, IR(163, 2), IR(34600, 11), IR(0, 0), IR(1, 0), IR(199, 1), SPV_SPIRV_VERSION_WORD(1,2), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpGroupNonUniformElect, true, true, IR(340, 3), IR(34611, 21), IR(0, 0), IR(77, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformAll, true, true, IR(274, 4), IR(34632, 19), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformAny, true, true, IR(274, 4), IR(34651, 19), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformAllEqual, true, true, IR(274, 4), IR(34670, 24), IR(0, 0), IR(239, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBroadcast, true, true, IR(278, 5), IR(34694, 25), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBroadcastFirst, true, true, IR(274, 4), IR(34719, 30), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallot, true, true, IR(274, 4), IR(6847, 22), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformInverseBallot, true, true, IR(274, 4), IR(34749, 29), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotBitExtract, true, true, IR(278, 5), IR(34778, 32), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotBitCount, true, true, IR(283, 5), IR(34810, 30), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotFindLSB, true, true, IR(274, 4), IR(34840, 29), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBallotFindMSB, true, true, IR(274, 4), IR(34869, 29), IR(0, 0), IR(240, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffle, true, true, IR(278, 5), IR(9986, 23), IR(0, 0), IR(241, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffleXor, true, true, IR(278, 5), IR(34898, 26), IR(0, 0), IR(241, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffleUp, true, true, IR(278, 5), IR(34924, 25), IR(0, 0), IR(242, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformShuffleDown, true, true, IR(278, 5), IR(34949, 27), IR(0, 0), IR(242, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformIAdd, true, true, IR(343, 6), IR(34976, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFAdd, true, true, IR(343, 6), IR(34996, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformIMul, true, true, IR(343, 6), IR(35016, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFMul, true, true, IR(343, 6), IR(35036, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformSMin, true, true, IR(343, 6), IR(35056, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformUMin, true, true, IR(343, 6), IR(35076, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFMin, true, true, IR(343, 6), IR(35096, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformSMax, true, true, IR(343, 6), IR(35116, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformUMax, true, true, IR(343, 6), IR(35136, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformFMax, true, true, IR(343, 6), IR(35156, 20), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBitwiseAnd, true, true, IR(343, 6), IR(35176, 26), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBitwiseOr, true, true, IR(343, 6), IR(35202, 25), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformBitwiseXor, true, true, IR(343, 6), IR(35227, 26), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformLogicalAnd, true, true, IR(343, 6), IR(35253, 26), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformLogicalOr, true, true, IR(343, 6), IR(35279, 25), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformLogicalXor, true, true, IR(343, 6), IR(35304, 26), IR(0, 0), IR(243, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformQuadBroadcast, true, true, IR(278, 5), IR(35330, 29), IR(0, 0), IR(246, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformQuadSwap, true, true, IR(278, 5), IR(35359, 24), IR(0, 0), IR(246, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,3), 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpCopyLogical, true, true, IR(160, 3), IR(35383, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpPtrEqual, true, true, IR(172, 4), IR(35395, 9), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPtrNotEqual, true, true, IR(172, 4), IR(35404, 12), IR(0, 0), IR(1, 0), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPtrDiff, true, true, IR(172, 4), IR(35416, 8), IR(0, 0), IR(247, 3), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpColorAttachmentReadEXT, true, true, IR(349, 4), IR(35424, 23), IR(0, 0), IR(156, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpDepthAttachmentReadEXT, true, true, IR(353, 3), IR(35447, 23), IR(0, 0), IR(165, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpStencilAttachmentReadEXT, true, true, IR(353, 3), IR(35470, 25), IR(0, 0), IR(166, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpTypeTensorARM, true, false, IR(356, 4), IR(35495, 14), IR(0, 0), IR(219, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTensorReadARM, true, true, IR(360, 5), IR(35551, 14), IR(0, 0), IR(219, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kTensor},
  {spv::Op::OpTensorWriteARM, false, false, IR(365, 4), IR(35565, 15), IR(0, 0), IR(219, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kTensor},
  {spv::Op::OpTensorQuerySizeARM, true, true, IR(172, 4), IR(35580, 19), IR(0, 0), IR(219, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kTensor},
  {spv::Op::OpGraphConstantARM, true, true, IR(369, 3), IR(35599, 17), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphEntryPointARM, false, false, IR(372, 3), IR(35616, 19), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphARM, true, true, IR(39, 2), IR(10277, 9), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphInputARM, true, true, IR(122, 4), IR(35635, 14), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphSetOutputARM, false, false, IR(375, 3), IR(35649, 18), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpGraphEndARM, false, false, IR(0, 0), IR(35667, 12), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGraph},
  {spv::Op::OpTypeGraphARM, true, false, IR(378, 3), IR(35679, 13), IR(0, 0), IR(250, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpBitcastExtractEXT, true, true, IR(172, 4), IR(10424, 18), IR(0, 0), IR(251, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpTerminateInvocation, false, false, IR(0, 0), IR(35692, 20), IR(0, 0), IR(3, 1), IR(209, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpTypeUntypedPointerKHR, true, false, IR(381, 2), IR(35712, 22), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpUntypedVariableKHR, true, true, IR(383, 5), IR(35734, 19), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedAccessChainKHR, true, true, IR(151, 5), IR(35753, 22), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedInBoundsAccessChainKHR, true, true, IR(151, 5), IR(35775, 30), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpSubgroupBallotKHR, true, true, IR(160, 3), IR(6608, 18), IR(0, 0), IR(252, 1), IR(2, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupFirstInvocationKHR, true, true, IR(160, 3), IR(35805, 27), IR(0, 0), IR(252, 1), IR(2, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpUntypedPtrAccessChainKHR, true, true, IR(388, 6), IR(35832, 25), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedInBoundsPtrAccessChainKHR, true, true, IR(388, 6), IR(35857, 33), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedArrayLengthKHR, true, true, IR(394, 5), IR(35890, 22), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpUntypedPrefetchKHR, false, false, IR(399, 5), IR(35912, 19), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpFmaKHR, true, true, IR(130, 5), IR(35931, 7), IR(0, 0), IR(253, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSubgroupAllKHR, true, true, IR(160, 3), IR(35938, 15), IR(0, 0), IR(254, 1), IR(42, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupAnyKHR, true, true, IR(160, 3), IR(35953, 15), IR(0, 0), IR(254, 1), IR(42, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupAllEqualKHR, true, true, IR(160, 3), IR(35968, 20), IR(0, 0), IR(254, 1), IR(42, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupNonUniformRotateKHR, true, true, IR(404, 6), IR(15284, 25), IR(0, 0), IR(255, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupReadInvocationKHR, true, true, IR(172, 4), IR(35988, 26), IR(0, 0), IR(252, 1), IR(2, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpExtInstWithForwardRefsKHR, true, true, IR(410, 5), IR(36014, 26), IR(0, 0), IR(1, 0), IR(210, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpUntypedGroupAsyncCopyKHR, true, true, IR(415, 11), IR(36040, 25), IR(0, 0), IR(93, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpTraceRayKHR, false, false, IR(426, 11), IR(36065, 12), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpExecuteCallableKHR, false, false, IR(36, 2), IR(36077, 19), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertUToAccelerationStructureKHR, true, true, IR(160, 3), IR(36096, 35), IR(0, 0), IR(256, 2), IR(211, 2), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIgnoreIntersectionKHR, false, false, IR(0, 0), IR(36131, 22), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTerminateRayKHR, false, false, IR(0, 0), IR(36153, 16), IR(0, 0), IR(61, 1), IR(28, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSDot, true, true, IR(437, 5), IR(36216, 5), IR(174, 1), IR(258, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUDot, true, true, IR(437, 5), IR(36229, 5), IR(175, 1), IR(258, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSUDot, true, true, IR(437, 5), IR(36242, 6), IR(176, 1), IR(258, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSDotAccSat, true, true, IR(442, 6), IR(36257, 11), IR(177, 1), IR(258, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpUDotAccSat, true, true, IR(442, 6), IR(36282, 11), IR(178, 1), IR(258, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpSUDotAccSat, true, true, IR(442, 6), IR(36307, 12), IR(179, 1), IR(258, 1), IR(153, 1), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpTypeCooperativeMatrixKHR, true, false, IR(448, 6), IR(36334, 25), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCooperativeMatrixLoadKHR, true, true, IR(454, 6), IR(36359, 25), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixStoreKHR, false, false, IR(460, 5), IR(36384, 26), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixMulAddKHR, true, true, IR(465, 6), IR(36464, 27), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpCooperativeMatrixLengthKHR, true, true, IR(160, 3), IR(36491, 27), IR(0, 0), IR(87, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpConstantCompositeReplicateEXT, true, true, IR(160, 3), IR(36518, 30), IR(0, 0), IR(259, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantCompositeReplicateEXT, true, true, IR(160, 3), IR(36548, 34), IR(0, 0), IR(259, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpCompositeConstructReplicateEXT, true, true, IR(160, 3), IR(36582, 31), IR(0, 0), IR(259, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpTypeRayQueryKHR, true, false, IR(68, 1), IR(36613, 16), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpRayQueryInitializeKHR, false, false, IR(471, 8), IR(36629, 22), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryTerminateKHR, false, false, IR(10, 1), IR(36651, 21), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGenerateIntersectionKHR, false, false, IR(36, 2), IR(36672, 32), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryConfirmIntersectionKHR, false, false, IR(10, 1), IR(36704, 31), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryProceedKHR, true, true, IR(160, 3), IR(36735, 19), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionTypeKHR, true, true, IR(172, 4), IR(36754, 31), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpImageSampleWeightedQCOM, true, true, IR(130, 5), IR(36785, 24), IR(0, 0), IR(260, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBoxFilterQCOM, true, true, IR(130, 5), IR(36809, 19), IR(0, 0), IR(261, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchSSDQCOM, true, true, IR(325, 7), IR(36828, 23), IR(0, 0), IR(262, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchSADQCOM, true, true, IR(325, 7), IR(36851, 23), IR(0, 0), IR(262, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpBitCastArrayQCOM, true, true, IR(160, 3), IR(36874, 17), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpImageBlockMatchWindowSSDQCOM, true, true, IR(325, 7), IR(36891, 29), IR(0, 0), IR(264, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchWindowSADQCOM, true, true, IR(325, 7), IR(36920, 29), IR(0, 0), IR(264, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchGatherSSDQCOM, true, true, IR(325, 7), IR(36949, 29), IR(0, 0), IR(264, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpImageBlockMatchGatherSADQCOM, true, true, IR(325, 7), IR(36978, 29), IR(0, 0), IR(264, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpCompositeConstructCoopMatQCOM, true, true, IR(160, 3), IR(37007, 30), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpCompositeExtractCoopMatQCOM, true, true, IR(160, 3), IR(37037, 28), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpExtractSubArrayQCOM, true, true, IR(172, 4), IR(37065, 20), IR(0, 0), IR(263, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpImageGatherQCOM, true, true, IR(479, 7), IR(37085, 16), IR(0, 0), IR(265, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpGroupIAddNonUniformAMD, true, true, IR(283, 5), IR(37101, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFAddNonUniformAMD, true, true, IR(283, 5), IR(37124, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMinNonUniformAMD, true, true, IR(283, 5), IR(37147, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMinNonUniformAMD, true, true, IR(283, 5), IR(37170, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMinNonUniformAMD, true, true, IR(283, 5), IR(37193, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMaxNonUniformAMD, true, true, IR(283, 5), IR(37216, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupUMaxNonUniformAMD, true, true, IR(283, 5), IR(37239, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupSMaxNonUniformAMD, true, true, IR(283, 5), IR(37262, 23), IR(0, 0), IR(235, 1), IR(33, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpFragmentMaskFetchAMD, true, true, IR(172, 4), IR(37285, 21), IR(0, 0), IR(267, 1), IR(61, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFragmentFetchAMD, true, true, IR(130, 5), IR(37306, 17), IR(0, 0), IR(267, 1), IR(61, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReadClockKHR, true, true, IR(340, 3), IR(37323, 13), IR(0, 0), IR(268, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAllocateNodePayloadsAMDX, true, true, IR(278, 5), IR(37336, 25), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpEnqueueNodePayloadsAMDX, false, false, IR(10, 1), IR(37361, 24), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeNodePayloadArrayAMDX, true, false, IR(87, 2), IR(37385, 25), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFinishWritingNodePayloadAMDX, true, true, IR(160, 3), IR(37410, 29), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpNodePayloadArrayLengthAMDX, true, true, IR(160, 3), IR(37439, 27), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIsNodePayloadValidAMDX, true, true, IR(172, 4), IR(37466, 23), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConstantStringAMDX, true, false, IR(50, 2), IR(37489, 19), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSpecConstantStringAMDX, true, false, IR(50, 2), IR(37508, 23), IR(0, 0), IR(43, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpGroupNonUniformQuadAllKHR, true, true, IR(160, 3), IR(37531, 26), IR(0, 0), IR(173, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpGroupNonUniformQuadAnyKHR, true, true, IR(160, 3), IR(37557, 26), IR(0, 0), IR(173, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpTypeBufferEXT, true, false, IR(381, 2), IR(37583, 14), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpBufferPointerEXT, true, true, IR(160, 3), IR(37597, 17), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpAbortKHR, false, false, IR(36, 2), IR(11498, 9), IR(0, 0), IR(269, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpUntypedImageTexelPointerEXT, true, true, IR(215, 6), IR(37614, 28), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpMemberDecorateIdEXT, false, false, IR(165, 3), IR(37642, 20), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpConstantSizeOfEXT, true, true, IR(160, 3), IR(37662, 18), IR(0, 0), IR(44, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConstantDataKHR, true, true, IR(486, 3), IR(11507, 16), IR(0, 0), IR(127, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantDataKHR, true, true, IR(486, 3), IR(37680, 20), IR(0, 0), IR(127, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpPoisonKHR, true, true, IR(39, 2), IR(37700, 10), IR(0, 0), IR(174, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpFreezeKHR, true, true, IR(160, 3), IR(37710, 10), IR(0, 0), IR(174, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpHitObjectRecordHitMotionNV, false, false, IR(489, 14), IR(37720, 27), IR(0, 0), IR(270, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordHitWithIndexMotionNV, false, false, IR(503, 13), IR(37747, 36), IR(0, 0), IR(270, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissMotionNV, false, false, IR(516, 7), IR(37783, 28), IR(0, 0), IR(270, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldToObjectNV, true, true, IR(160, 3), IR(37811, 28), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectToWorldNV, true, true, IR(160, 3), IR(37839, 28), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayDirectionNV, true, true, IR(160, 3), IR(37867, 33), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayOriginNV, true, true, IR(160, 3), IR(37900, 30), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayMotionNV, false, false, IR(503, 13), IR(37930, 26), IR(0, 0), IR(270, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderRecordBufferHandleNV, true, true, IR(160, 3), IR(37956, 39), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderBindingTableRecordIndexNV, true, true, IR(160, 3), IR(37995, 44), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordEmptyNV, false, false, IR(10, 1), IR(38039, 23), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayNV, false, false, IR(523, 12), IR(38062, 20), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordHitNV, false, false, IR(503, 13), IR(38082, 21), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordHitWithIndexNV, false, false, IR(523, 12), IR(38103, 30), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissNV, false, false, IR(535, 6), IR(38133, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectExecuteShaderNV, false, false, IR(36, 2), IR(38155, 25), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetCurrentTimeNV, true, true, IR(160, 3), IR(38180, 26), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetAttributesNV, false, false, IR(36, 2), IR(38206, 25), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetHitKindNV, true, true, IR(160, 3), IR(38231, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetPrimitiveIndexNV, true, true, IR(160, 3), IR(38253, 29), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetGeometryIndexNV, true, true, IR(160, 3), IR(38282, 28), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceIdNV, true, true, IR(160, 3), IR(38310, 25), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceCustomIndexNV, true, true, IR(160, 3), IR(38335, 34), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayDirectionNV, true, true, IR(160, 3), IR(38369, 32), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayOriginNV, true, true, IR(160, 3), IR(38401, 29), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMaxNV, true, true, IR(160, 3), IR(38430, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMinNV, true, true, IR(160, 3), IR(38452, 22), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsEmptyNV, true, true, IR(160, 3), IR(38474, 19), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsHitNV, true, true, IR(160, 3), IR(38493, 17), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsMissNV, true, true, IR(160, 3), IR(38510, 18), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReorderThreadWithHitObjectNV, false, false, IR(541, 3), IR(38528, 29), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReorderThreadWithHintNV, false, false, IR(36, 2), IR(38557, 24), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeHitObjectNV, true, false, IR(68, 1), IR(38581, 16), IR(0, 0), IR(133, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpImageSampleFootprintNV, true, true, IR(479, 7), IR(38597, 23), IR(0, 0), IR(272, 1), IR(80, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpTypeVectorIdEXT, true, false, IR(89, 3), IR(38620, 16), IR(180, 1), IR(273, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCooperativeVectorMatrixMulNV, true, true, IR(544, 13), IR(38660, 29), IR(0, 0), IR(275, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorOuterProductAccumulateNV, false, false, IR(557, 7), IR(38689, 42), IR(0, 0), IR(276, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorReduceSumAccumulateNV, false, false, IR(33, 3), IR(38731, 39), IR(0, 0), IR(276, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorMatrixMulAddNV, true, true, IR(564, 16), IR(38770, 32), IR(0, 0), IR(275, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixConvertNV, true, true, IR(160, 3), IR(38802, 27), IR(0, 0), IR(277, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpEmitMeshTasksEXT, false, false, IR(580, 4), IR(38829, 17), IR(0, 0), IR(54, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSetMeshOutputsEXT, false, false, IR(36, 2), IR(38846, 18), IR(0, 0), IR(54, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpGroupNonUniformPartitionEXT, true, true, IR(160, 3), IR(38864, 28), IR(181, 1), IR(199, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kNon_Uniform},
  {spv::Op::OpWritePackedPrimitiveIndices4x8NV, false, false, IR(36, 2), IR(38919, 33), IR(0, 0), IR(51, 1), IR(18, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFetchMicroTriangleVertexPositionNV, true, true, IR(325, 7), IR(38952, 35), IR(0, 0), IR(278, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFetchMicroTriangleVertexBarycentricNV, true, true, IR(325, 7), IR(38987, 38), IR(0, 0), IR(278, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeVectorLoadNV, true, true, IR(584, 5), IR(39025, 24), IR(0, 0), IR(275, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeVectorStoreNV, false, false, IR(589, 4), IR(39049, 25), IR(0, 0), IR(275, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpHitObjectRecordFromQueryEXT, false, false, IR(593, 5), IR(39074, 28), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissEXT, false, false, IR(516, 7), IR(39102, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordMissMotionEXT, false, false, IR(471, 8), IR(39125, 29), IR(0, 0), IR(279, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetIntersectionTriangleVertexPositionsEXT, true, true, IR(160, 3), IR(39154, 51), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayFlagsEXT, true, true, IR(160, 3), IR(39205, 24), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectSetShaderBindingTableRecordIndexEXT, false, false, IR(36, 2), IR(39229, 45), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectReorderExecuteShaderEXT, false, false, IR(598, 4), IR(39274, 33), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceReorderExecuteEXT, false, false, IR(602, 14), IR(39307, 32), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceMotionReorderExecuteEXT, false, false, IR(616, 15), IR(39339, 38), IR(0, 0), IR(279, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeHitObjectEXT, true, false, IR(68, 1), IR(39377, 17), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpReorderThreadWithHintEXT, false, false, IR(36, 2), IR(39394, 25), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReorderThreadWithHitObjectEXT, false, false, IR(541, 3), IR(39419, 30), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayEXT, false, false, IR(523, 12), IR(39449, 21), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectTraceRayMotionEXT, false, false, IR(503, 13), IR(39470, 27), IR(0, 0), IR(279, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectRecordEmptyEXT, false, false, IR(10, 1), IR(39497, 24), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectExecuteShaderEXT, false, false, IR(36, 2), IR(39521, 26), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetCurrentTimeEXT, true, true, IR(160, 3), IR(39547, 27), IR(0, 0), IR(279, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetAttributesEXT, false, false, IR(36, 2), IR(39574, 26), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetHitKindEXT, true, true, IR(160, 3), IR(39600, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetPrimitiveIndexEXT, true, true, IR(160, 3), IR(39623, 30), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetGeometryIndexEXT, true, true, IR(160, 3), IR(39653, 29), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceIdEXT, true, true, IR(160, 3), IR(39682, 26), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetInstanceCustomIndexEXT, true, true, IR(160, 3), IR(39708, 35), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayOriginEXT, true, true, IR(160, 3), IR(39743, 31), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectRayDirectionEXT, true, true, IR(160, 3), IR(39774, 34), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayDirectionEXT, true, true, IR(160, 3), IR(39808, 33), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldRayOriginEXT, true, true, IR(160, 3), IR(39841, 30), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetObjectToWorldEXT, true, true, IR(160, 3), IR(39871, 29), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetWorldToObjectEXT, true, true, IR(160, 3), IR(39900, 29), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMaxEXT, true, true, IR(160, 3), IR(39929, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpReportIntersectionKHR, true, true, IR(172, 4), IR(39952, 22), IR(182, 1), IR(55, 2), IR(24, 2), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIgnoreIntersectionNV, false, false, IR(0, 0), IR(39995, 21), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTerminateRayNV, false, false, IR(0, 0), IR(40016, 15), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTraceNV, false, false, IR(426, 11), IR(40031, 8), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTraceMotionNV, false, false, IR(523, 12), IR(40039, 14), IR(0, 0), IR(58, 1), IR(27, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTraceRayMotionNV, false, false, IR(523, 12), IR(40053, 17), IR(0, 0), IR(58, 1), IR(27, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionTriangleVertexPositionsKHR, true, true, IR(172, 4), IR(40070, 50), IR(0, 0), IR(281, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeAccelerationStructureKHR, true, false, IR(68, 1), IR(40120, 29), IR(183, 1), IR(282, 4), IR(213, 4), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpExecuteCallableNV, false, false, IR(36, 2), IR(40177, 18), IR(0, 0), IR(57, 1), IR(26, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionClusterIdNV, true, true, IR(172, 4), IR(40195, 35), IR(184, 1), IR(65, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetClusterIdNV, true, true, IR(160, 3), IR(40253, 24), IR(0, 0), IR(65, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetRayTMinEXT, true, true, IR(160, 3), IR(40277, 23), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderBindingTableRecordIndexEXT, true, true, IR(160, 3), IR(40300, 45), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetShaderRecordBufferHandleEXT, true, true, IR(160, 3), IR(40345, 40), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsEmptyEXT, true, true, IR(160, 3), IR(40385, 20), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsHitEXT, true, true, IR(160, 3), IR(40405, 18), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsMissEXT, true, true, IR(160, 3), IR(40423, 19), IR(0, 0), IR(134, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTypeCooperativeMatrixNV, true, false, IR(631, 5), IR(40442, 24), IR(0, 0), IR(286, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCooperativeMatrixLoadNV, true, true, IR(636, 6), IR(40466, 24), IR(0, 0), IR(286, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixStoreNV, false, false, IR(642, 5), IR(40490, 25), IR(0, 0), IR(286, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixMulAddNV, true, true, IR(130, 5), IR(40515, 26), IR(0, 0), IR(286, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixLengthNV, true, true, IR(160, 3), IR(40541, 26), IR(0, 0), IR(286, 1), IR(88, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpBeginInvocationInterlockEXT, false, false, IR(0, 0), IR(40567, 28), IR(0, 0), IR(287, 3), IR(89, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpEndInvocationInterlockEXT, false, false, IR(0, 0), IR(40595, 26), IR(0, 0), IR(287, 3), IR(89, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixReduceNV, true, true, IR(647, 5), IR(40664, 26), IR(0, 0), IR(290, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kArithmetic},
  {spv::Op::OpCooperativeMatrixLoadTensorNV, true, true, IR(652, 7), IR(40765, 30), IR(0, 0), IR(217, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixStoreTensorNV, false, false, IR(659, 5), IR(40795, 31), IR(0, 0), IR(217, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpCooperativeMatrixPerElementOpNV, true, true, IR(151, 5), IR(40826, 32), IR(0, 0), IR(291, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kFunction},
  {spv::Op::OpTypeTensorLayoutNV, true, false, IR(89, 3), IR(40858, 19), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeTensorViewNV, true, false, IR(664, 4), IR(40877, 17), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpCreateTensorLayoutNV, true, true, IR(39, 2), IR(40894, 21), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetDimensionNV, true, true, IR(122, 4), IR(40915, 27), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetStrideNV, true, true, IR(122, 4), IR(40942, 24), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSliceNV, true, true, IR(122, 4), IR(40966, 20), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetClampValueNV, true, true, IR(172, 4), IR(40986, 28), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCreateTensorViewNV, true, true, IR(39, 2), IR(41014, 19), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorViewSetDimensionNV, true, true, IR(122, 4), IR(41033, 25), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorViewSetStrideNV, true, true, IR(122, 4), IR(41058, 22), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpDemoteToHelperInvocation, false, false, IR(0, 0), IR(13038, 25), IR(46, 1), IR(293, 1), IR(0, 0), SPV_SPIRV_VERSION_WORD(1,6), 0xffffffffu, PrintingClass::kControl_Flow},
  {spv::Op::OpIsHelperInvocationEXT, true, true, IR(39, 2), IR(41080, 22), IR(0, 0), IR(293, 1), IR(90, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorViewSetClipNV, true, true, IR(325, 7), IR(41102, 20), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpTensorLayoutSetBlockSizeNV, true, true, IR(122, 4), IR(41122, 27), IR(0, 0), IR(292, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpCooperativeMatrixTransposeNV, true, true, IR(160, 3), IR(41149, 29), IR(0, 0), IR(277, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertUToImageNV, true, true, IR(160, 3), IR(41178, 18), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertUToSamplerNV, true, true, IR(160, 3), IR(41196, 20), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertImageToUNV, true, true, IR(160, 3), IR(41216, 18), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertSamplerToUNV, true, true, IR(160, 3), IR(41234, 20), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertUToSampledImageNV, true, true, IR(160, 3), IR(41254, 25), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConvertSampledImageToUNV, true, true, IR(160, 3), IR(41279, 25), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSamplerImageAddressingModeNV, false, false, IR(0, 1), IR(41304, 29), IR(0, 0), IR(135, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRawAccessChainNV, true, true, IR(668, 7), IR(41385, 17), IR(0, 0), IR(206, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpRayQueryGetIntersectionSpherePositionNV, true, true, IR(172, 4), IR(41402, 40), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionSphereRadiusNV, true, true, IR(172, 4), IR(41442, 38), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionLSSPositionsNV, true, true, IR(172, 4), IR(41480, 38), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionLSSRadiiNV, true, true, IR(172, 4), IR(41518, 34), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionLSSHitValueNV, true, true, IR(172, 4), IR(41552, 37), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetSpherePositionNV, true, true, IR(160, 3), IR(41589, 29), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetSphereRadiusNV, true, true, IR(160, 3), IR(41618, 27), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetLSSPositionsNV, true, true, IR(160, 3), IR(41645, 27), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectGetLSSRadiiNV, true, true, IR(160, 3), IR(41672, 23), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsSphereHitNV, true, true, IR(160, 3), IR(41695, 23), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpHitObjectIsLSSHitNV, true, true, IR(160, 3), IR(41718, 20), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryIsSphereHitNV, true, true, IR(172, 4), IR(41738, 22), IR(0, 0), IR(62, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryIsLSSHitNV, true, true, IR(172, 4), IR(41760, 19), IR(0, 0), IR(63, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpSubgroupShuffleINTEL, true, true, IR(172, 4), IR(13631, 21), IR(0, 0), IR(294, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupShuffleDownINTEL, true, true, IR(130, 5), IR(41779, 25), IR(0, 0), IR(294, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupShuffleUpINTEL, true, true, IR(130, 5), IR(41804, 23), IR(0, 0), IR(294, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupShuffleXorINTEL, true, true, IR(172, 4), IR(41827, 24), IR(0, 0), IR(294, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupBlockReadINTEL, true, true, IR(160, 3), IR(41851, 23), IR(0, 0), IR(295, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupBlockWriteINTEL, false, false, IR(36, 2), IR(41874, 24), IR(0, 0), IR(295, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageBlockReadINTEL, true, true, IR(172, 4), IR(41898, 28), IR(0, 0), IR(296, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageBlockWriteINTEL, false, false, IR(33, 3), IR(41926, 29), IR(0, 0), IR(296, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageMediaBlockReadINTEL, true, true, IR(215, 6), IR(41955, 33), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupImageMediaBlockWriteINTEL, false, false, IR(675, 5), IR(41988, 34), IR(0, 0), IR(297, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpUCountLeadingZerosINTEL, true, true, IR(160, 3), IR(42022, 24), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUCountTrailingZerosINTEL, true, true, IR(160, 3), IR(42046, 25), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAbsISubINTEL, true, true, IR(172, 4), IR(42071, 13), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAbsUSubINTEL, true, true, IR(172, 4), IR(42084, 13), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIAddSatINTEL, true, true, IR(172, 4), IR(42097, 13), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUAddSatINTEL, true, true, IR(172, 4), IR(42110, 13), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIAverageINTEL, true, true, IR(172, 4), IR(42123, 14), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUAverageINTEL, true, true, IR(172, 4), IR(42137, 14), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIAverageRoundedINTEL, true, true, IR(172, 4), IR(42151, 21), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUAverageRoundedINTEL, true, true, IR(172, 4), IR(42172, 21), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpISubSatINTEL, true, true, IR(172, 4), IR(42193, 13), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUSubSatINTEL, true, true, IR(172, 4), IR(42206, 13), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpIMul32x16INTEL, true, true, IR(172, 4), IR(42219, 15), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpUMul32x16INTEL, true, true, IR(172, 4), IR(42234, 15), IR(0, 0), IR(298, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpConstantFunctionPointerINTEL, true, true, IR(160, 3), IR(42249, 29), IR(0, 0), IR(215, 1), IR(110, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFunctionPointerCallINTEL, true, true, IR(107, 3), IR(42278, 25), IR(0, 0), IR(215, 1), IR(110, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAsmTargetINTEL, true, false, IR(50, 2), IR(42303, 15), IR(0, 0), IR(138, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAsmINTEL, true, true, IR(680, 6), IR(13849, 9), IR(0, 0), IR(138, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAsmCallINTEL, true, true, IR(122, 4), IR(42318, 13), IR(0, 0), IR(138, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAtomicFMinEXT, true, true, IR(235, 6), IR(42331, 14), IR(0, 0), IR(299, 4), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAtomicFMaxEXT, true, true, IR(235, 6), IR(42345, 14), IR(0, 0), IR(299, 4), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpAssumeTrueKHR, false, false, IR(10, 1), IR(42359, 14), IR(0, 0), IR(303, 1), IR(114, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpExpectKHR, true, true, IR(172, 4), IR(42373, 10), IR(0, 0), IR(303, 1), IR(114, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpDecorateString, false, false, IR(163, 2), IR(42383, 15), IR(185, 1), IR(1, 0), IR(217, 2), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpMemberDecorateString, false, false, IR(165, 3), IR(42419, 21), IR(186, 1), IR(1, 0), IR(217, 2), SPV_SPIRV_VERSION_WORD(1,4), 0xffffffffu, PrintingClass::kAnnotation},
  {spv::Op::OpVmeImageINTEL, true, true, IR(172, 4), IR(42467, 14), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeVmeImageINTEL, true, false, IR(87, 2), IR(42481, 18), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImePayloadINTEL, true, false, IR(68, 1), IR(42499, 23), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcRefPayloadINTEL, true, false, IR(68, 1), IR(42522, 23), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcSicPayloadINTEL, true, false, IR(68, 1), IR(42545, 23), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcMcePayloadINTEL, true, false, IR(68, 1), IR(42568, 23), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcMceResultINTEL, true, false, IR(68, 1), IR(42591, 22), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeResultINTEL, true, false, IR(68, 1), IR(42613, 22), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeResultSingleReferenceStreamoutINTEL, true, false, IR(68, 1), IR(42635, 46), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeResultDualReferenceStreamoutINTEL, true, false, IR(68, 1), IR(42681, 44), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeSingleReferenceStreaminINTEL, true, false, IR(68, 1), IR(42725, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcImeDualReferenceStreaminINTEL, true, false, IR(68, 1), IR(42764, 37), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcRefResultINTEL, true, false, IR(68, 1), IR(42801, 22), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeAvcSicResultINTEL, true, false, IR(68, 1), IR(42823, 22), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL, true, true, IR(172, 4), IR(42845, 60), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL, true, true, IR(172, 4), IR(42905, 53), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL, true, true, IR(172, 4), IR(42958, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetInterShapePenaltyINTEL, true, true, IR(172, 4), IR(43005, 40), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL, true, true, IR(172, 4), IR(43045, 51), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetInterDirectionPenaltyINTEL, true, true, IR(172, 4), IR(43096, 44), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL, true, true, IR(172, 4), IR(43140, 51), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL, true, true, IR(172, 4), IR(43191, 56), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL, true, true, IR(39, 2), IR(43247, 50), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL, true, true, IR(39, 2), IR(43297, 52), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL, true, true, IR(39, 2), IR(43349, 49), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL, true, true, IR(215, 6), IR(43398, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL, true, true, IR(172, 4), IR(43445, 50), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL, true, true, IR(39, 2), IR(43495, 51), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL, true, true, IR(39, 2), IR(43546, 56), IR(0, 0), IR(307, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetAcOnlyHaarINTEL, true, true, IR(160, 3), IR(43602, 33), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL, true, true, IR(172, 4), IR(43635, 52), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL, true, true, IR(172, 4), IR(43687, 61), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL, true, true, IR(130, 5), IR(43748, 61), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToImePayloadINTEL, true, true, IR(160, 3), IR(43809, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToImeResultINTEL, true, true, IR(160, 3), IR(43848, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToRefPayloadINTEL, true, true, IR(160, 3), IR(43886, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToRefResultINTEL, true, true, IR(160, 3), IR(43925, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToSicPayloadINTEL, true, true, IR(160, 3), IR(43963, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceConvertToSicResultINTEL, true, true, IR(160, 3), IR(44002, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetMotionVectorsINTEL, true, true, IR(160, 3), IR(44040, 36), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterDistortionsINTEL, true, true, IR(160, 3), IR(44076, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetBestInterDistortionsINTEL, true, true, IR(160, 3), IR(44115, 43), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterMajorShapeINTEL, true, true, IR(160, 3), IR(44158, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterMinorShapeINTEL, true, true, IR(160, 3), IR(44196, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterDirectionsINTEL, true, true, IR(160, 3), IR(44234, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterMotionVectorCountINTEL, true, true, IR(160, 3), IR(44272, 45), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterReferenceIdsINTEL, true, true, IR(160, 3), IR(44317, 40), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL, true, true, IR(130, 5), IR(44357, 62), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeInitializeINTEL, true, true, IR(130, 5), IR(44419, 30), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetSingleReferenceINTEL, true, true, IR(130, 5), IR(44449, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetDualReferenceINTEL, true, true, IR(215, 6), IR(44487, 36), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeRefWindowSizeINTEL, true, true, IR(172, 4), IR(44523, 33), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeAdjustRefOffsetINTEL, true, true, IR(215, 6), IR(44556, 35), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeConvertToMcePayloadINTEL, true, true, IR(160, 3), IR(44591, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetMaxMotionVectorCountINTEL, true, true, IR(172, 4), IR(44630, 43), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL, true, true, IR(160, 3), IR(44673, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, true, true, IR(172, 4), IR(44720, 54), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeSetWeightedSadINTEL, true, true, IR(172, 4), IR(44774, 34), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL, true, true, IR(130, 5), IR(44808, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceINTEL, true, true, IR(215, 6), IR(44855, 45), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL, true, true, IR(215, 6), IR(44900, 55), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL, true, true, IR(325, 7), IR(44955, 53), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL, true, true, IR(130, 5), IR(45008, 56), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL, true, true, IR(215, 6), IR(45064, 54), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL, true, true, IR(215, 6), IR(45118, 58), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL, true, true, IR(325, 7), IR(45176, 56), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeConvertToMceResultINTEL, true, true, IR(160, 3), IR(45232, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetSingleReferenceStreaminINTEL, true, true, IR(160, 3), IR(45270, 46), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetDualReferenceStreaminINTEL, true, true, IR(160, 3), IR(45316, 44), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL, true, true, IR(160, 3), IR(45360, 49), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeStripDualReferenceStreamoutINTEL, true, true, IR(160, 3), IR(45409, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL, true, true, IR(172, 4), IR(45456, 70), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL, true, true, IR(172, 4), IR(45526, 68), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL, true, true, IR(172, 4), IR(45594, 69), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, true, true, IR(130, 5), IR(45663, 68), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, true, true, IR(130, 5), IR(45731, 66), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, true, true, IR(130, 5), IR(45797, 67), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetBorderReachedINTEL, true, true, IR(172, 4), IR(45864, 36), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL, true, true, IR(160, 3), IR(45900, 48), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL, true, true, IR(160, 3), IR(45948, 59), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL, true, true, IR(160, 3), IR(46007, 58), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL, true, true, IR(160, 3), IR(46065, 56), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcFmeInitializeINTEL, true, true, IR(686, 9), IR(46121, 30), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcBmeInitializeINTEL, true, true, IR(695, 10), IR(46151, 30), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefConvertToMcePayloadINTEL, true, true, IR(160, 3), IR(46181, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefSetBidirectionalMixDisableINTEL, true, true, IR(160, 3), IR(46220, 46), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefSetBilinearFilterEnableINTEL, true, true, IR(160, 3), IR(46266, 43), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL, true, true, IR(130, 5), IR(46309, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithDualReferenceINTEL, true, true, IR(215, 6), IR(46356, 45), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL, true, true, IR(130, 5), IR(46401, 46), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL, true, true, IR(215, 6), IR(46447, 56), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcRefConvertToMceResultINTEL, true, true, IR(160, 3), IR(46503, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicInitializeINTEL, true, true, IR(160, 3), IR(46541, 30), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConfigureSkcINTEL, true, true, IR(288, 8), IR(46571, 32), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConfigureIpeLumaINTEL, true, true, IR(695, 10), IR(46603, 36), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConfigureIpeLumaChromaINTEL, true, true, IR(705, 13), IR(46639, 42), IR(0, 0), IR(307, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetMotionVectorMaskINTEL, true, true, IR(172, 4), IR(46681, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConvertToMcePayloadINTEL, true, true, IR(160, 3), IR(46720, 39), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL, true, true, IR(172, 4), IR(46759, 44), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL, true, true, IR(215, 6), IR(46803, 48), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL, true, true, IR(172, 4), IR(46851, 50), IR(0, 0), IR(307, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetBilinearFilterEnableINTEL, true, true, IR(160, 3), IR(46901, 43), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL, true, true, IR(172, 4), IR(46944, 48), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL, true, true, IR(172, 4), IR(46992, 43), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateIpeINTEL, true, true, IR(172, 4), IR(47035, 31), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL, true, true, IR(130, 5), IR(47066, 47), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithDualReferenceINTEL, true, true, IR(215, 6), IR(47113, 45), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL, true, true, IR(130, 5), IR(47158, 46), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL, true, true, IR(215, 6), IR(47204, 56), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicConvertToMceResultINTEL, true, true, IR(160, 3), IR(47260, 38), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetIpeLumaShapeINTEL, true, true, IR(160, 3), IR(47298, 35), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL, true, true, IR(160, 3), IR(47333, 44), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL, true, true, IR(160, 3), IR(47377, 46), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetPackedIpeLumaModesINTEL, true, true, IR(160, 3), IR(47423, 41), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetIpeChromaModeINTEL, true, true, IR(160, 3), IR(47464, 36), IR(0, 0), IR(307, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL, true, true, IR(160, 3), IR(47500, 50), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL, true, true, IR(160, 3), IR(47550, 48), IR(0, 0), IR(305, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupAvcSicGetInterRawSadsINTEL, true, true, IR(160, 3), IR(47598, 35), IR(0, 0), IR(304, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpVariableLengthArrayINTEL, true, true, IR(160, 3), IR(14087, 25), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpSaveMemoryINTEL, true, true, IR(39, 2), IR(47633, 16), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpRestoreMemoryINTEL, false, false, IR(10, 1), IR(47649, 19), IR(0, 0), IR(309, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpArbitraryFloatSinCosPiALTERA, true, true, IR(718, 8), IR(47668, 29), IR(187, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCastALTERA, true, true, IR(718, 8), IR(47725, 25), IR(188, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCastFromIntALTERA, true, true, IR(718, 8), IR(47774, 32), IR(189, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCastToIntALTERA, true, true, IR(718, 8), IR(47837, 30), IR(190, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatAddALTERA, true, true, IR(726, 10), IR(47896, 24), IR(191, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSubALTERA, true, true, IR(726, 10), IR(47943, 24), IR(192, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatMulALTERA, true, true, IR(726, 10), IR(47990, 24), IR(193, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatDivALTERA, true, true, IR(726, 10), IR(48037, 24), IR(194, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatGTALTERA, true, true, IR(736, 6), IR(48084, 23), IR(195, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatGEALTERA, true, true, IR(736, 6), IR(48129, 23), IR(196, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLTALTERA, true, true, IR(736, 6), IR(48174, 23), IR(197, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLEALTERA, true, true, IR(736, 6), IR(48219, 23), IR(198, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatEQALTERA, true, true, IR(736, 6), IR(48264, 23), IR(199, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatRecipALTERA, true, true, IR(718, 8), IR(48309, 26), IR(200, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatRSqrtALTERA, true, true, IR(718, 8), IR(48360, 26), IR(201, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCbrtALTERA, true, true, IR(718, 8), IR(48411, 25), IR(202, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatHypotALTERA, true, true, IR(726, 10), IR(48460, 26), IR(203, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSqrtALTERA, true, true, IR(718, 8), IR(48511, 25), IR(204, 1), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLogINTEL, true, true, IR(718, 8), IR(48560, 23), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLog2INTEL, true, true, IR(718, 8), IR(48583, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLog10INTEL, true, true, IR(718, 8), IR(48607, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatLog1pINTEL, true, true, IR(718, 8), IR(48632, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExpINTEL, true, true, IR(718, 8), IR(48657, 23), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExp2INTEL, true, true, IR(718, 8), IR(48680, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExp10INTEL, true, true, IR(718, 8), IR(48704, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatExpm1INTEL, true, true, IR(718, 8), IR(48729, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSinINTEL, true, true, IR(718, 8), IR(48754, 23), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCosINTEL, true, true, IR(718, 8), IR(48777, 23), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSinCosINTEL, true, true, IR(718, 8), IR(48800, 26), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatSinPiINTEL, true, true, IR(718, 8), IR(48826, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatCosPiINTEL, true, true, IR(718, 8), IR(48851, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatASinINTEL, true, true, IR(718, 8), IR(48876, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatASinPiINTEL, true, true, IR(718, 8), IR(48900, 26), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatACosINTEL, true, true, IR(718, 8), IR(48926, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatACosPiINTEL, true, true, IR(718, 8), IR(48950, 26), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatATanINTEL, true, true, IR(718, 8), IR(48976, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatATanPiINTEL, true, true, IR(718, 8), IR(49000, 26), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatATan2INTEL, true, true, IR(726, 10), IR(49026, 25), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatPowINTEL, true, true, IR(726, 10), IR(49051, 23), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatPowRINTEL, true, true, IR(726, 10), IR(49074, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpArbitraryFloatPowNINTEL, true, true, IR(726, 10), IR(49098, 24), IR(0, 0), IR(310, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpLoopControlINTEL, false, false, IR(18, 1), IR(49122, 17), IR(0, 0), IR(311, 1), IR(124, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAliasDomainDeclINTEL, true, false, IR(742, 2), IR(49139, 21), IR(0, 0), IR(144, 1), IR(136, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAliasScopeDeclINTEL, true, false, IR(744, 3), IR(49160, 20), IR(0, 0), IR(144, 1), IR(136, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpAliasScopeListDeclINTEL, true, false, IR(92, 2), IR(49180, 24), IR(0, 0), IR(144, 1), IR(136, 1), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSqrtALTERA, true, true, IR(718, 8), IR(49204, 16), IR(205, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedRecipALTERA, true, true, IR(718, 8), IR(49235, 17), IR(206, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedRsqrtALTERA, true, true, IR(718, 8), IR(49268, 17), IR(207, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinALTERA, true, true, IR(718, 8), IR(49301, 15), IR(208, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedCosALTERA, true, true, IR(718, 8), IR(49330, 15), IR(209, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinCosALTERA, true, true, IR(718, 8), IR(49359, 18), IR(210, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinPiALTERA, true, true, IR(718, 8), IR(49394, 17), IR(211, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedCosPiALTERA, true, true, IR(718, 8), IR(49427, 17), IR(212, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedSinCosPiALTERA, true, true, IR(718, 8), IR(49460, 20), IR(213, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedLogALTERA, true, true, IR(718, 8), IR(49499, 15), IR(214, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFixedExpALTERA, true, true, IR(718, 8), IR(49528, 15), IR(215, 1), IR(205, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpPtrCastToCrossWorkgroupALTERA, true, true, IR(160, 3), IR(49557, 30), IR(216, 1), IR(216, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpCrossWorkgroupCastToPtrALTERA, true, true, IR(160, 3), IR(49616, 30), IR(217, 1), IR(216, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpReadPipeBlockingALTERA, false, false, IR(296, 4), IR(49675, 23), IR(218, 1), IR(312, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpWritePipeBlockingALTERA, false, false, IR(296, 4), IR(49720, 24), IR(219, 1), IR(312, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpFPGARegALTERA, true, true, IR(160, 3), IR(15045, 14), IR(63, 1), IR(313, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpRayQueryGetRayTMinKHR, true, true, IR(160, 3), IR(49767, 22), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetRayFlagsKHR, true, true, IR(160, 3), IR(49789, 23), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionTKHR, true, true, IR(172, 4), IR(49812, 28), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionInstanceCustomIndexKHR, true, true, IR(172, 4), IR(49840, 46), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionInstanceIdKHR, true, true, IR(172, 4), IR(49886, 37), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR, true, true, IR(172, 4), IR(49923, 65), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionGeometryIndexKHR, true, true, IR(172, 4), IR(49988, 40), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionPrimitiveIndexKHR, true, true, IR(172, 4), IR(50028, 41), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionBarycentricsKHR, true, true, IR(172, 4), IR(50069, 39), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionFrontFaceKHR, true, true, IR(172, 4), IR(50108, 36), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionCandidateAABBOpaqueKHR, true, true, IR(160, 3), IR(50144, 46), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionObjectRayDirectionKHR, true, true, IR(172, 4), IR(50190, 45), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionObjectRayOriginKHR, true, true, IR(172, 4), IR(50235, 42), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetWorldRayDirectionKHR, true, true, IR(160, 3), IR(50277, 32), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetWorldRayOriginKHR, true, true, IR(160, 3), IR(50309, 29), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionObjectToWorldKHR, true, true, IR(172, 4), IR(50338, 40), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpRayQueryGetIntersectionWorldToObjectKHR, true, true, IR(172, 4), IR(50378, 40), IR(0, 0), IR(209, 1), IR(49, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpAtomicFAddEXT, true, true, IR(235, 6), IR(50418, 14), IR(0, 0), IR(314, 4), IR(161, 1), 0xffffffffu, 0xffffffffu, PrintingClass::kAtomic},
  {spv::Op::OpTypeBufferSurfaceINTEL, true, false, IR(100, 2), IR(50432, 23), IR(0, 0), IR(136, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpTypeStructContinuedINTEL, false, false, IR(747, 1), IR(50455, 25), IR(0, 0), IR(318, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kType_Declaration},
  {spv::Op::OpConstantCompositeContinuedINTEL, false, false, IR(747, 1), IR(50480, 32), IR(0, 0), IR(318, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantCompositeContinuedINTEL, false, false, IR(747, 1), IR(50512, 36), IR(0, 0), IR(318, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpCompositeConstructContinuedINTEL, false, false, IR(747, 1), IR(50548, 33), IR(0, 0), IR(318, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpConvertFToBF16INTEL, true, true, IR(160, 3), IR(50581, 20), IR(0, 0), IR(319, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpConvertBF16ToFINTEL, true, true, IR(160, 3), IR(50601, 20), IR(0, 0), IR(319, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpControlBarrierArriveEXT, false, false, IR(221, 3), IR(50621, 24), IR(220, 1), IR(320, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpControlBarrierWaitEXT, false, false, IR(221, 3), IR(50671, 22), IR(221, 1), IR(320, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kBarrier},
  {spv::Op::OpArithmeticFenceEXT, true, true, IR(160, 3), IR(15556, 19), IR(0, 0), IR(321, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMiscellaneous},
  {spv::Op::OpTaskSequenceCreateALTERA, true, true, IR(748, 7), IR(50717, 25), IR(222, 1), IR(322, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTaskSequenceAsyncALTERA, false, false, IR(168, 2), IR(50766, 24), IR(223, 1), IR(322, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTaskSequenceGetALTERA, true, true, IR(160, 3), IR(50813, 22), IR(224, 1), IR(322, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTaskSequenceReleaseALTERA, false, false, IR(10, 1), IR(50856, 26), IR(225, 1), IR(322, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpTypeTaskSequenceALTERA, true, false, IR(68, 1), IR(50907, 23), IR(226, 1), IR(322, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::k_exclude},
  {spv::Op::OpSubgroupBlockPrefetchINTEL, false, false, IR(139, 3), IR(50952, 27), IR(0, 0), IR(323, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockLoadINTEL, false, false, IR(755, 10), IR(50979, 25), IR(0, 0), IR(112, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockLoadTransformINTEL, false, false, IR(755, 10), IR(51004, 34), IR(0, 0), IR(324, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockLoadTransposeINTEL, false, false, IR(755, 10), IR(51038, 34), IR(0, 0), IR(325, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockPrefetchINTEL, false, false, IR(765, 9), IR(51072, 29), IR(0, 0), IR(112, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroup2DBlockStoreINTEL, false, false, IR(755, 10), IR(51101, 26), IR(0, 0), IR(112, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpSubgroupMatrixMultiplyAccumulateINTEL, true, true, IR(774, 7), IR(16033, 38), IR(0, 0), IR(326, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpBitwiseFunctionINTEL, true, true, IR(215, 6), IR(51189, 21), IR(0, 0), IR(327, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kBit},
  {spv::Op::OpUntypedVariableLengthArrayINTEL, true, true, IR(172, 4), IR(16099, 32), IR(0, 0), IR(328, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpConditionalExtensionINTEL, false, false, IR(45, 2), IR(51210, 26), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kExtension},
  {spv::Op::OpConditionalEntryPointINTEL, false, false, IR(781, 5), IR(51236, 27), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpConditionalCapabilityINTEL, false, false, IR(786, 2), IR(51263, 27), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMode_Setting},
  {spv::Op::OpSpecConstantTargetINTEL, true, true, IR(788, 4), IR(51290, 24), IR(0, 0), IR(329, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantArchitectureINTEL, true, true, IR(792, 6), IR(51314, 30), IR(0, 0), IR(329, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpSpecConstantCapabilitiesINTEL, true, true, IR(798, 3), IR(51381, 30), IR(0, 0), IR(329, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConstant_Creation},
  {spv::Op::OpConditionalCopyObjectINTEL, true, true, IR(107, 3), IR(51411, 27), IR(0, 0), IR(115, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kComposite},
  {spv::Op::OpPredicatedLoadINTEL, true, true, IR(636, 6), IR(51438, 20), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpPredicatedStoreINTEL, false, false, IR(589, 4), IR(51458, 21), IR(0, 0), IR(330, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpGroupIMulKHR, true, true, IR(283, 5), IR(51479, 13), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupFMulKHR, true, true, IR(283, 5), IR(51492, 13), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBitwiseAndKHR, true, true, IR(283, 5), IR(51505, 19), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBitwiseOrKHR, true, true, IR(283, 5), IR(51524, 18), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupBitwiseXorKHR, true, true, IR(283, 5), IR(51542, 19), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupLogicalAndKHR, true, true, IR(283, 5), IR(51561, 19), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupLogicalOrKHR, true, true, IR(283, 5), IR(51580, 18), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpGroupLogicalXorKHR, true, true, IR(283, 5), IR(51598, 19), IR(0, 0), IR(331, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kGroup},
  {spv::Op::OpRoundFToTF32INTEL, true, true, IR(160, 3), IR(51617, 18), IR(0, 0), IR(332, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kConversion},
  {spv::Op::OpMaskedGatherINTEL, true, true, IR(801, 6), IR(51635, 18), IR(0, 0), IR(333, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpMaskedScatterINTEL, false, false, IR(807, 4), IR(51653, 19), IR(0, 0), IR(333, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kMemory},
  {spv::Op::OpConvertHandleToImageINTEL, true, true, IR(160, 3), IR(51672, 26), IR(0, 0), IR(334, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpConvertHandleToSamplerINTEL, true, true, IR(160, 3), IR(51698, 28), IR(0, 0), IR(334, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpConvertHandleToSampledImageINTEL, true, true, IR(160, 3), IR(51726, 33), IR(0, 0), IR(334, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kImage},
  {spv::Op::OpFDot2MixAcc32VALVE, true, true, IR(130, 5), IR(51759, 19), IR(0, 0), IR(335, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFDot2MixAcc16VALVE, true, true, IR(130, 5), IR(51778, 19), IR(0, 0), IR(337, 2), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
  {spv::Op::OpFDot4MixAcc32VALVE, true, true, IR(130, 5), IR(51797, 19), IR(0, 0), IR(339, 1), IR(0, 0), 0xffffffffu, 0xffffffffu, PrintingClass::kReserved},
}};





static const std::array<NameIndex, 930> kInstructionNames{{
  {IR(11498, 9), 440},
  {IR(42071, 13), 602},
  {IR(42084, 13), 603},
  {IR(30644, 12), 58},
  {IR(49139, 21), 788},
  {IR(49160, 20), 789},
  {IR(49180, 24), 790},
  {IR(31941, 4), 143},
  {IR(37336, 25), 428},
  {IR(31937, 4), 142},
  {IR(48926, 24), 779},
  {IR(48950, 26), 780},
  {IR(48876, 24), 777},
  {IR(48900, 26), 778},
  {IR(49026, 25), 783},
  {IR(48976, 24), 781},
  {IR(49000, 26), 782},
  {IR(47896, 24), 750},
  {IR(47920, 23), 750},
  {IR(47725, 25), 747},
  {IR(47774, 32), 748},
  {IR(47806, 31), 748},
  {IR(47750, 24), 747},
  {IR(47837, 30), 749},
  {IR(47867, 29), 749},
  {IR(48411, 25), 761},
  {IR(48436, 24), 761},
  {IR(48777, 23), 773},
  {IR(48851, 25), 776},
  {IR(48037, 24), 753},
  {IR(48061, 23), 753},
  {IR(48264, 23), 758},
  {IR(48287, 22), 758},
  {IR(48704, 25), 770},
  {IR(48680, 24), 769},
  {IR(48657, 23), 768},
  {IR(48729, 25), 771},
  {IR(48129, 23), 755},
  {IR(48152, 22), 755},
  {IR(48084, 23), 754},
  {IR(48107, 22), 754},
  {IR(48460, 26), 762},
  {IR(48486, 25), 762},
  {IR(48219, 23), 757},
  {IR(48242, 22), 757},
  {IR(48174, 23), 756},
  {IR(48197, 22), 756},
  {IR(48607, 25), 766},
  {IR(48632, 25), 767},
  {IR(48583, 24), 765},
  {IR(48560, 23), 764},
  {IR(47990, 24), 752},
  {IR(48014, 23), 752},
  {IR(49051, 23), 784},
  {IR(49098, 24), 786},
  {IR(49074, 24), 785},
  {IR(48360, 26), 760},
  {IR(48386, 25), 760},
  {IR(48309, 26), 759},
  {IR(48335, 25), 759},
  {IR(48800, 26), 774},
  {IR(47668, 29), 746},
  {IR(47697, 28), 746},
  {IR(48754, 23), 772},
  {IR(48826, 25), 775},
  {IR(48511, 25), 763},
  {IR(48536, 24), 763},
  {IR(47943, 24), 751},
  {IR(47967, 23), 751},
  {IR(15556, 19), 834},
  {IR(30691, 12), 61},
  {IR(42318, 13), 618},
  {IR(13849, 9), 617},
  {IR(42303, 15), 616},
  {IR(42359, 14), 621},
  {IR(32963, 10), 220},
  {IR(32815, 22), 210},
  {IR(32837, 26), 211},
  {IR(32800, 15), 209},
  {IR(50418, 14), 824},
  {IR(42345, 14), 620},
  {IR(42331, 14), 619},
  {IR(34348, 16), 292},
  {IR(34327, 21), 291},
  {IR(32897, 11), 214},
  {IR(32880, 17), 213},
  {IR(32863, 17), 212},
  {IR(32908, 11), 215},
  {IR(32777, 11), 207},
  {IR(32973, 9), 221},
  {IR(32941, 11), 218},
  {IR(32919, 11), 216},
  {IR(32788, 12), 208},
  {IR(32952, 11), 219},
  {IR(32930, 11), 217},
  {IR(32982, 10), 222},
  {IR(40567, 28), 548},
  {IR(36874, 17), 408},
  {IR(32561, 9), 191},
  {IR(32501, 15), 187},
  {IR(32516, 17), 188},
  {IR(32533, 17), 189},
  {IR(32550, 11), 190},
  {IR(31689, 8), 114},
  {IR(10424, 18), 358},
  {IR(32486, 11), 185},
  {IR(51189, 21), 847},
  {IR(32465, 10), 183},
  {IR(32475, 11), 184},
  {IR(33092, 7), 227},
  {IR(33099, 18), 228},
  {IR(37597, 17), 439},
  {IR(33960, 13), 277},
  {IR(29705, 11), 15},
  {IR(33918, 26), 275},
  {IR(35424, 23), 344},
  {IR(33510, 15), 255},
  {IR(33525, 16), 256},
  {IR(30951, 19), 72},
  {IR(50548, 33), 829},
  {IR(37007, 30), 413},
  {IR(36582, 31), 396},
  {IR(30970, 17), 73},
  {IR(37037, 28), 414},
  {IR(30987, 16), 74},
  {IR(51263, 27), 851},
  {IR(51411, 27), 855},
  {IR(51236, 27), 850},
  {IR(51210, 26), 849},
  {IR(17872, 9), 39},
  {IR(30211, 18), 40},
  {IR(50480, 32), 827},
  {IR(36518, 30), 394},
  {IR(11507, 16), 444},
  {IR(30159, 14), 38},
  {IR(42249, 29), 614},
  {IR(30323, 13), 42},
  {IR(34403, 20), 296},
  {IR(30307, 16), 41},
  {IR(37662, 18), 443},
  {IR(37489, 19), 434},
  {IR(30146, 13), 37},
  {IR(32748, 15), 205},
  {IR(50621, 24), 832},
  {IR(50645, 26), 832},
  {IR(50671, 22), 833},
  {IR(50693, 24), 833},
  {IR(50601, 20), 831},
  {IR(50581, 20), 830},
  {IR(31495, 12), 100},
  {IR(31483, 12), 99},
  {IR(51672, 26), 869},
  {IR(51726, 33), 871},
  {IR(51698, 28), 870},
  {IR(41216, 18), 571},
  {IR(31572, 14), 107},
  {IR(31507, 12), 101},
  {IR(41279, 25), 574},
  {IR(41234, 20), 572},
  {IR(36096, 35), 380},
  {IR(31519, 12), 102},
  {IR(41178, 18), 569},
  {IR(31616, 14), 110},
  {IR(41254, 25), 573},
  {IR(41196, 20), 570},
  {IR(38802, 27), 487},
  {IR(36491, 27), 393},
  {IR(40541, 26), 547},
  {IR(36359, 25), 390},
  {IR(40466, 24), 544},
  {IR(40765, 30), 551},
  {IR(36464, 27), 392},
  {IR(40515, 26), 546},
  {IR(40826, 32), 553},
  {IR(40664, 26), 550},
  {IR(36384, 26), 391},
  {IR(40490, 25), 545},
  {IR(40795, 31), 552},
  {IR(41149, 29), 568},
  {IR(39025, 24), 494},
  {IR(38770, 32), 486},
  {IR(38660, 29), 483},
  {IR(38689, 42), 484},
  {IR(38731, 39), 485},
  {IR(39049, 25), 495},
  {IR(35383, 12), 340},
  {IR(30617, 11), 56},
  {IR(30628, 16), 57},
  {IR(31003, 11), 75},
  {IR(34423, 26), 297},
  {IR(40894, 21), 556},
  {IR(41014, 19), 561},
  {IR(33870, 16), 272},
  {IR(49616, 30), 803},
  {IR(49646, 29), 803},
  {IR(32570, 5), 192},
  {IR(32616, 11), 198},
  {IR(32587, 9), 195},
  {IR(32575, 5), 193},
  {IR(32627, 11), 199},
  {IR(32596, 9), 196},
  {IR(30777, 9), 64},
  {IR(34600, 11), 305},
  {IR(42383, 15), 623},
  {IR(42398, 21), 623},
  {IR(30801, 16), 66},
  {IR(13038, 25), 564},
  {IR(13063, 28), 564},
  {IR(35447, 23), 345},
  {IR(31886, 4), 137},
  {IR(38829, 17), 488},
  {IR(32675, 17), 203},
  {IR(32651, 11), 201},
  {IR(40595, 26), 549},
  {IR(32662, 13), 202},
  {IR(32692, 19), 204},
  {IR(33706, 14), 265},
  {IR(33692, 14), 264},
  {IR(37361, 24), 429},
  {IR(29620, 11), 13},
  {IR(36077, 19), 379},
  {IR(40177, 18), 534},
  {IR(29663, 14), 14},
  {IR(34584, 16), 304},
  {IR(42373, 10), 622},
  {IR(29474, 8), 11},
  {IR(29414, 14), 10},
  {IR(36014, 26), 376},
  {IR(29404, 10), 9},
  {IR(37065, 20), 415},
  {IR(31718, 5), 118},
  {IR(31549, 9), 105},
  {IR(31753, 5), 125},
  {IR(51778, 19), 873},
  {IR(51759, 19), 872},
  {IR(51797, 19), 874},
  {IR(31778, 5), 130},
  {IR(31738, 5), 122},
  {IR(31705, 8), 116},
  {IR(32215, 10), 168},
  {IR(32293, 16), 174},
  {IR(32365, 21), 178},
  {IR(32265, 13), 172},
  {IR(32327, 18), 176},
  {IR(32237, 13), 170},
  {IR(15045, 14), 806},
  {IR(15059, 13), 806},
  {IR(31773, 5), 129},
  {IR(31728, 5), 120},
  {IR(32225, 12), 169},
  {IR(32309, 18), 175},
  {IR(32386, 23), 179},
  {IR(32278, 15), 173},
  {IR(32345, 20), 177},
  {IR(32250, 15), 171},
  {IR(38987, 38), 493},
  {IR(38952, 35), 492},
  {IR(37410, 29), 431},
  {IR(49330, 15), 795},
  {IR(49345, 14), 795},
  {IR(49427, 17), 798},
  {IR(49444, 16), 798},
  {IR(49528, 15), 801},
  {IR(49543, 14), 801},
  {IR(49499, 15), 800},
  {IR(49514, 14), 800},
  {IR(49235, 17), 792},
  {IR(49252, 16), 792},
  {IR(49268, 17), 793},
  {IR(49285, 16), 793},
  {IR(49301, 15), 794},
  {IR(49359, 18), 796},
  {IR(49377, 17), 796},
  {IR(49460, 20), 799},
  {IR(49480, 19), 799},
  {IR(49316, 14), 794},
  {IR(49394, 17), 797},
  {IR(49411, 16), 797},
  {IR(49204, 16), 791},
  {IR(49220, 15), 791},
  {IR(35931, 7), 370},
  {IR(37306, 17), 426},
  {IR(37285, 21), 425},
  {IR(37710, 10), 447},
  {IR(28453, 9), 48},
  {IR(30526, 13), 51},
  {IR(30514, 12), 50},
  {IR(30496, 18), 49},
  {IR(42278, 25), 615},
  {IR(32580, 7), 194},
  {IR(32638, 13), 200},
  {IR(32605, 11), 197},
  {IR(31647, 17), 112},
  {IR(31664, 25), 113},
  {IR(30703, 23), 62},
  {IR(33944, 16), 276},
  {IR(34449, 35), 298},
  {IR(34484, 25), 299},
  {IR(33750, 32), 267},
  {IR(33720, 30), 266},
  {IR(33805, 40), 269},
  {IR(33782, 23), 268},
  {IR(33576, 18), 259},
  {IR(33558, 18), 258},
  {IR(10277, 9), 353},
  {IR(35599, 17), 351},
  {IR(35667, 12), 356},
  {IR(35616, 19), 352},
  {IR(35635, 14), 354},
  {IR(35649, 18), 355},
  {IR(33263, 9), 238},
  {IR(33272, 9), 239},
  {IR(33232, 15), 236},
  {IR(51505, 19), 860},
  {IR(51524, 18), 861},
  {IR(51542, 19), 862},
  {IR(33281, 15), 240},
  {IR(33651, 20), 262},
  {IR(33671, 21), 263},
  {IR(30817, 14), 67},
  {IR(33339, 10), 242},
  {IR(37124, 23), 418},
  {IR(33379, 10), 246},
  {IR(37216, 23), 422},
  {IR(33349, 10), 243},
  {IR(37147, 23), 419},
  {IR(51492, 13), 859},
  {IR(33329, 10), 241},
  {IR(37101, 23), 417},
  {IR(51479, 13), 858},
  {IR(51561, 19), 863},
  {IR(51580, 18), 864},
  {IR(51598, 19), 865},
  {IR(30876, 20), 68},
  {IR(34632, 19), 307},
  {IR(34670, 24), 309},
  {IR(34651, 19), 308},
  {IR(6847, 22), 312},
  {IR(34810, 30), 315},
  {IR(34778, 32), 314},
  {IR(34840, 29), 316},
  {IR(34869, 29), 317},
  {IR(35176, 26), 332},
  {IR(35202, 25), 333},
  {IR(35227, 26), 334},
  {IR(34694, 25), 310},
  {IR(34719, 30), 311},
  {IR(34611, 21), 306},
  {IR(34996, 20), 323},
  {IR(35156, 20), 331},
  {IR(35096, 20), 328},
  {IR(35036, 20), 325},
  {IR(34976, 20), 322},
  {IR(35016, 20), 324},
  {IR(34749, 29), 313},
  {IR(35253, 26), 335},
  {IR(35279, 25), 336},
  {IR(35304, 26), 337},
  {IR(38864, 28), 490},
  {IR(38892, 27), 490},
  {IR(37531, 26), 436},
  {IR(37557, 26), 437},
  {IR(35330, 29), 338},
  {IR(35359, 24), 339},
  {IR(15284, 25), 374},
  {IR(35116, 20), 329},
  {IR(35056, 20), 326},
  {IR(9986, 23), 318},
  {IR(34949, 27), 321},
  {IR(34924, 25), 320},
  {IR(34898, 26), 319},
  {IR(35136, 20), 330},
  {IR(35076, 20), 327},
  {IR(33594, 28), 260},
  {IR(33622, 29), 261},
  {IR(33399, 10), 248},
  {IR(37262, 23), 424},
  {IR(33369, 10), 245},
  {IR(37193, 23), 421},
  {IR(33389, 10), 247},
  {IR(37239, 23), 423},
  {IR(33359, 10), 244},
  {IR(37170, 23), 420},
  {IR(33247, 16), 237},
  {IR(39521, 26), 511},
  {IR(38155, 25), 463},
  {IR(39574, 26), 513},
  {IR(38206, 25), 465},
  {IR(40253, 24), 536},
  {IR(39547, 27), 512},
  {IR(38180, 26), 464},
  {IR(39653, 29), 516},
  {IR(38282, 28), 468},
  {IR(39600, 23), 514},
  {IR(38231, 22), 466},
  {IR(39708, 35), 518},
  {IR(38335, 34), 470},
  {IR(39682, 26), 517},
  {IR(38310, 25), 469},
  {IR(39154, 51), 499},
  {IR(41645, 27), 584},
  {IR(41672, 23), 585},
  {IR(39774, 34), 520},
  {IR(37867, 33), 453},
  {IR(39743, 31), 519},
  {IR(37900, 30), 454},
  {IR(39871, 29), 523},
  {IR(37839, 28), 452},
  {IR(39623, 30), 515},
  {IR(38253, 29), 467},
  {IR(39205, 24), 500},
  {IR(39929, 23), 525},
  {IR(38430, 22), 473},
  {IR(40277, 23), 537},
  {IR(38452, 22), 474},
  {IR(40300, 45), 538},
  {IR(37995, 44), 457},
  {IR(40345, 40), 539},
  {IR(37956, 39), 456},
  {IR(41589, 29), 582},
  {IR(41618, 27), 583},
  {IR(39808, 33), 521},
  {IR(38369, 32), 471},
  {IR(39841, 30), 522},
  {IR(38401, 29), 472},
  {IR(39900, 29), 524},
  {IR(37811, 28), 451},
  {IR(40385, 20), 540},
  {IR(38474, 19), 475},
  {IR(40405, 18), 541},
  {IR(38493, 17), 476},
  {IR(41718, 20), 587},
  {IR(40423, 19), 542},
  {IR(38510, 18), 477},
  {IR(41695, 23), 586},
  {IR(39497, 24), 510},
  {IR(38039, 23), 458},
  {IR(39074, 28), 496},
  {IR(37720, 27), 448},
  {IR(38082, 21), 460},
  {IR(37747, 36), 449},
  {IR(38103, 30), 461},
  {IR(39102, 23), 497},
  {IR(39125, 29), 498},
  {IR(37783, 28), 450},
  {IR(38133, 22), 462},
  {IR(39274, 33), 502},
  {IR(39229, 45), 501},
  {IR(39339, 38), 504},
  {IR(39449, 21), 508},
  {IR(39470, 27), 509},
  {IR(37930, 26), 455},
  {IR(38062, 20), 459},
  {IR(39307, 32), 503},
  {IR(31713, 5), 117},
  {IR(31890, 10), 138},
  {IR(42097, 13), 604},
  {IR(42123, 14), 606},
  {IR(42151, 21), 608},
  {IR(32086, 7), 158},
  {IR(31733, 5), 121},
  {IR(42219, 15), 612},
  {IR(32093, 10), 159},
  {IR(31723, 5), 119},
  {IR(31900, 11), 139},
  {IR(42193, 13), 610},
  {IR(36131, 22), 381},
  {IR(39995, 21), 527},
  {IR(28497, 6), 91},
  {IR(36978, 29), 412},
  {IR(36949, 29), 411},
  {IR(36851, 23), 407},
  {IR(36828, 23), 406},
  {IR(36920, 29), 410},
  {IR(36891, 29), 409},
  {IR(36809, 19), 405},
  {IR(31331, 16), 88},
  {IR(31308, 11), 86},
  {IR(31319, 12), 87},
  {IR(37085, 16), 416},
  {IR(31368, 17), 92},
  {IR(31448, 17), 97},
  {IR(31434, 14), 96},
  {IR(31385, 16), 93},
  {IR(31465, 18), 98},
  {IR(31419, 15), 95},
  {IR(31401, 18), 94},
  {IR(31347, 10), 89},
  {IR(31165, 27), 81},
  {IR(31138, 27), 80},
  {IR(31115, 23), 79},
  {IR(38597, 23), 481},
  {IR(31069, 23), 78},
  {IR(31277, 31), 85},
  {IR(31246, 31), 84},
  {IR(31219, 27), 83},
  {IR(31192, 27), 82},
  {IR(36785, 24), 404},
  {IR(34272, 22), 288},
  {IR(34237, 17), 286},
  {IR(34254, 18), 287},
  {IR(34364, 16), 293},
  {IR(34064, 33), 281},
  {IR(34031, 33), 280},
  {IR(34002, 29), 279},
  {IR(33973, 29), 278},
  {IR(34200, 37), 285},
  {IR(34163, 37), 284},
  {IR(34130, 33), 283},
  {IR(34097, 33), 282},
  {IR(34294, 26), 289},
  {IR(30548, 18), 53},
  {IR(31357, 11), 90},
  {IR(30656, 20), 59},
  {IR(30726, 23), 63},
  {IR(31957, 9), 146},
  {IR(41080, 22), 565},
  {IR(31951, 6), 145},
  {IR(31945, 6), 144},
  {IR(37466, 23), 433},
  {IR(31966, 9), 147},
  {IR(33886, 13), 273},
  {IR(33541, 17), 257},
  {IR(33169, 5), 230},
  {IR(33086, 6), 226},
  {IR(31986, 14), 149},
  {IR(33205, 14), 234},
  {IR(33219, 13), 235},
  {IR(29399, 5), 8},
  {IR(30606, 5), 54},
  {IR(32057, 11), 155},
  {IR(32018, 13), 152},
  {IR(32068, 11), 156},
  {IR(32031, 16), 153},
  {IR(32047, 10), 154},
  {IR(49122, 17), 787},
  {IR(33026, 10), 224},
  {IR(51635, 18), 867},
  {IR(51653, 19), 868},
  {IR(31855, 18), 135},
  {IR(31801, 18), 132},
  {IR(31837, 18), 134},
  {IR(30786, 15), 65},
  {IR(37642, 20), 442},
  {IR(42419, 21), 624},
  {IR(42440, 27), 624},
  {IR(29381, 11), 6},
  {IR(32763, 14), 206},
  {IR(29546, 12), 12},
  {IR(34549, 19), 302},
  {IR(34568, 16), 303},
  {IR(29376, 5), 5},
  {IR(34526, 23), 301},
  {IR(34320, 7), 290},
  {IR(37439, 27), 432},
  {IR(29172, 4), 0},
  {IR(32497, 4), 186},
  {IR(32000, 8), 150},
  {IR(31873, 13), 136},
  {IR(32992, 4), 223},
  {IR(37700, 10), 446},
  {IR(51438, 20), 856},
  {IR(51458, 21), 857},
  {IR(30676, 15), 60},
  {IR(49557, 30), 802},
  {IR(49587, 29), 802},
  {IR(31630, 17), 111},
  {IR(35416, 8), 343},
  {IR(35395, 9), 341},
  {IR(35404, 12), 342},
  {IR(31558, 14), 106},
  {IR(41385, 17), 576},
  {IR(36704, 31), 401},
  {IR(36672, 32), 400},
  {IR(40230, 23), 535},
  {IR(50069, 39), 815},
  {IR(50144, 46), 817},
  {IR(40195, 35), 535},
  {IR(50108, 36), 816},
  {IR(49988, 40), 813},
  {IR(49840, 46), 810},
  {IR(49886, 37), 811},
  {IR(49923, 65), 812},
  {IR(41552, 37), 581},
  {IR(41480, 38), 579},
  {IR(41518, 34), 580},
  {IR(50190, 45), 818},
  {IR(50235, 42), 819},
  {IR(50338, 40), 822},
  {IR(50028, 41), 814},
  {IR(41402, 40), 577},
  {IR(41442, 38), 578},
  {IR(49812, 28), 809},
  {IR(40070, 50), 532},
  {IR(36754, 31), 403},
  {IR(50378, 40), 823},
  {IR(49789, 23), 808},
  {IR(49767, 22), 807},
  {IR(50277, 32), 820},
  {IR(50309, 29), 821},
  {IR(36629, 22), 398},
  {IR(41760, 19), 589},
  {IR(41738, 22), 588},
  {IR(36735, 19), 402},
  {IR(36651, 21), 399},
  {IR(37323, 13), 427},
  {IR(33409, 9), 249},
  {IR(49675, 23), 804},
  {IR(49698, 22), 804},
  {IR(33857, 13), 271},
  {IR(39394, 25), 506},
  {IR(38557, 24), 479},
  {IR(39419, 30), 507},
  {IR(38528, 29), 478},
  {IR(39952, 22), 526},
  {IR(39974, 21), 526},
  {IR(33463, 23), 253},
  {IR(33486, 24), 254},
  {IR(33428, 17), 251},
  {IR(33445, 18), 252},
  {IR(47649, 19), 745},
  {IR(33845, 12), 270},
  {IR(33174, 7), 231},
  {IR(33181, 12), 232},
  {IR(51617, 18), 866},
  {IR(31540, 9), 104},
  {IR(31748, 5), 124},
  {IR(36216, 5), 383},
  {IR(36257, 11), 386},
  {IR(36268, 14), 386},
  {IR(36221, 8), 383},
  {IR(32116, 13), 161},
  {IR(32147, 18), 163},
  {IR(32175, 10), 165},
  {IR(32200, 15), 167},
  {IR(31768, 5), 128},
  {IR(31924, 13), 141},
  {IR(31697, 8), 115},
  {IR(31763, 5), 127},
  {IR(36242, 6), 385},
  {IR(36307, 12), 388},
  {IR(36319, 15), 388},
  {IR(36248, 9), 385},
  {IR(31024, 13), 77},
  {IR(41304, 29), 575},
  {IR(31586, 15), 108},
  {IR(31601, 15), 109},
  {IR(47633, 16), 744},
  {IR(32079, 7), 157},
  {IR(33071, 15), 225},
  {IR(38846, 18), 489},
  {IR(33899, 19), 274},
  {IR(32448, 17), 182},
  {IR(32427, 21), 181},
  {IR(32409, 18), 180},
  {IR(31975, 11), 148},
  {IR(34380, 7), 294},
  {IR(29353, 7), 3},
  {IR(29234, 16), 2},
  {IR(29360, 16), 4},
  {IR(30371, 13), 45},
  {IR(51314, 30), 853},
  {IR(51381, 30), 854},
  {IR(30384, 22), 46},
  {IR(50512, 36), 828},
  {IR(36548, 34), 395},
  {IR(37680, 20), 445},
  {IR(30353, 18), 44},
  {IR(30447, 15), 47},
  {IR(37508, 23), 435},
  {IR(51290, 24), 852},
  {IR(30336, 17), 43},
  {IR(35470, 25), 346},
  {IR(30611, 6), 55},
  {IR(29392, 7), 7},
  {IR(50979, 25), 841},
  {IR(51004, 34), 842},
  {IR(51038, 34), 843},
  {IR(51072, 29), 844},
  {IR(51101, 26), 845},
  {IR(35968, 20), 373},
  {IR(35938, 15), 371},
  {IR(35953, 15), 372},
  {IR(46151, 30), 708},
  {IR(46121, 30), 707},
  {IR(44556, 35), 677},
  {IR(44591, 39), 678},
  {IR(45232, 38), 691},
  {IR(44855, 45), 684},
  {IR(44955, 53), 686},
  {IR(45176, 56), 690},
  {IR(45064, 54), 688},
  {IR(44808, 47), 683},
  {IR(44900, 55), 685},
  {IR(45118, 58), 689},
  {IR(45008, 56), 687},
  {IR(45864, 36), 702},
  {IR(45316, 44), 693},
  {IR(45270, 46), 692},
  {IR(45731, 66), 700},
  {IR(45663, 68), 699},
  {IR(45797, 67), 701},
  {IR(45526, 68), 697},
  {IR(45456, 70), 696},
  {IR(45594, 69), 698},
  {IR(45900, 48), 703},
  {IR(45948, 59), 704},
  {IR(46065, 56), 706},
  {IR(46007, 58), 705},
  {IR(44419, 30), 673},
  {IR(44523, 33), 676},
  {IR(44487, 36), 675},
  {IR(44720, 54), 681},
  {IR(44630, 43), 679},
  {IR(44449, 38), 674},
  {IR(44673, 47), 680},
  {IR(44774, 34), 682},
  {IR(45409, 47), 695},
  {IR(45360, 49), 694},
  {IR(43809, 39), 658},
  {IR(43848, 38), 659},
  {IR(43886, 39), 660},
  {IR(43925, 38), 661},
  {IR(43963, 39), 662},
  {IR(44002, 38), 663},
  {IR(44115, 43), 666},
  {IR(43247, 50), 647},
  {IR(42845, 60), 639},
  {IR(43045, 51), 643},
  {IR(43191, 56), 646},
  {IR(42958, 47), 641},
  {IR(43546, 56), 653},
  {IR(43445, 50), 651},
  {IR(43140, 51), 645},
  {IR(43349, 49), 649},
  {IR(43297, 52), 648},
  {IR(43495, 51), 652},
  {IR(44234, 38), 669},
  {IR(44076, 39), 665},
  {IR(44158, 38), 667},
  {IR(44196, 38), 668},
  {IR(44272, 45), 670},
  {IR(44317, 40), 671},
  {IR(44357, 62), 672},
  {IR(44040, 36), 664},
  {IR(43602, 33), 654},
  {IR(43748, 61), 657},
  {IR(42905, 53), 640},
  {IR(43096, 44), 644},
  {IR(43005, 40), 642},
  {IR(43398, 47), 650},
  {IR(43687, 61), 656},
  {IR(43635, 52), 655},
  {IR(46181, 39), 709},
  {IR(46503, 38), 716},
  {IR(46356, 45), 713},
  {IR(46401, 46), 714},
  {IR(46447, 56), 715},
  {IR(46309, 47), 712},
  {IR(46220, 46), 710},
  {IR(46266, 43), 711},
  {IR(46639, 42), 720},
  {IR(46603, 36), 719},
  {IR(46571, 32), 718},
  {IR(46720, 39), 722},
  {IR(47260, 38), 734},
  {IR(47035, 31), 729},
  {IR(47113, 45), 731},
  {IR(47158, 46), 732},
  {IR(47204, 56), 733},
  {IR(47066, 47), 730},
  {IR(47377, 46), 737},
  {IR(47333, 44), 736},
  {IR(47598, 35), 742},
  {IR(47464, 36), 739},
  {IR(47298, 35), 735},
  {IR(46681, 39), 721},
  {IR(47423, 41), 738},
  {IR(47500, 50), 740},
  {IR(47550, 48), 741},
  {IR(46541, 30), 717},
  {IR(46901, 43), 726},
  {IR(46992, 43), 728},
  {IR(46851, 50), 725},
  {IR(46803, 48), 724},
  {IR(46759, 44), 723},
  {IR(46944, 48), 727},
  {IR(6608, 18), 364},
  {IR(50952, 27), 840},
  {IR(41851, 23), 594},
  {IR(41874, 24), 595},
  {IR(35805, 27), 365},
  {IR(41898, 28), 596},
  {IR(41926, 29), 597},
  {IR(41955, 33), 598},
  {IR(41988, 34), 599},
  {IR(16033, 38), 846},
  {IR(35988, 26), 375},
  {IR(41779, 25), 591},
  {IR(13631, 21), 590},
  {IR(41804, 23), 592},
  {IR(41827, 24), 593},
  {IR(33162, 7), 229},
  {IR(50766, 24), 836},
  {IR(50790, 23), 836},
  {IR(50717, 25), 835},
  {IR(50742, 24), 835},
  {IR(50813, 22), 837},
  {IR(50835, 21), 837},
  {IR(50856, 26), 838},
  {IR(50882, 25), 838},
  {IR(41122, 27), 567},
  {IR(40986, 28), 560},
  {IR(40915, 27), 557},
  {IR(40942, 24), 558},
  {IR(40966, 20), 559},
  {IR(35580, 19), 350},
  {IR(35551, 14), 348},
  {IR(41102, 20), 566},
  {IR(41033, 25), 562},
  {IR(41058, 22), 563},
  {IR(35565, 15), 349},
  {IR(35692, 20), 359},
  {IR(36153, 16), 382},
  {IR(40016, 15), 528},
  {IR(40039, 14), 530},
  {IR(40031, 8), 529},
  {IR(36065, 12), 378},
  {IR(40053, 17), 531},
  {IR(31014, 10), 76},
  {IR(40120, 29), 533},
  {IR(40149, 28), 533},
  {IR(29963, 10), 25},
  {IR(42764, 37), 636},
  {IR(42499, 23), 627},
  {IR(42681, 44), 634},
  {IR(42613, 22), 632},
  {IR(42635, 46), 633},
  {IR(42725, 39), 635},
  {IR(42568, 23), 630},
  {IR(42591, 22), 631},
  {IR(42522, 23), 628},
  {IR(42801, 22), 637},
  {IR(42545, 23), 629},
  {IR(42823, 22), 638},
  {IR(29725, 9), 17},
  {IR(37583, 14), 438},
  {IR(50432, 23), 825},
  {IR(36334, 25), 389},
  {IR(40442, 24), 543},
  {IR(38636, 24), 482},
  {IR(30078, 16), 32},
  {IR(30068, 10), 31},
  {IR(29779, 10), 19},
  {IR(30127, 19), 36},
  {IR(30055, 13), 30},
  {IR(35679, 13), 357},
  {IR(39377, 17), 505},
  {IR(38581, 16), 480},
  {IR(29924, 10), 22},
  {IR(29734, 8), 18},
  {IR(29800, 11), 21},
  {IR(34509, 17), 300},
  {IR(37385, 25), 430},
  {IR(30001, 11), 28},
  {IR(30118, 9), 35},
  {IR(34387, 16), 295},
  {IR(30043, 12), 29},
  {IR(30108, 10), 34},
  {IR(36613, 16), 397},
  {IR(30094, 14), 33},
  {IR(29973, 17), 26},
  {IR(29946, 17), 24},
  {IR(29934, 12), 23},
  {IR(29990, 11), 27},
  {IR(50455, 25), 826},
  {IR(50907, 23), 839},
  {IR(50930, 22), 839},
  {IR(35495, 14), 347},
  {IR(40858, 19), 554},
  {IR(40877, 17), 555},
  {IR(35712, 22), 360},
  {IR(29789, 11), 20},
  {IR(38620, 16), 482},
  {IR(42481, 18), 626},
  {IR(29716, 9), 16},
  {IR(42110, 13), 605},
  {IR(42137, 14), 607},
  {IR(42172, 21), 609},
  {IR(31531, 9), 103},
  {IR(42022, 24), 600},
  {IR(42046, 25), 601},
  {IR(31743, 5), 123},
  {IR(36229, 5), 384},
  {IR(36282, 11), 387},
  {IR(36293, 14), 387},
  {IR(36234, 8), 384},
  {IR(32103, 13), 160},
  {IR(32129, 18), 162},
  {IR(32165, 10), 164},
  {IR(32185, 15), 166},
  {IR(31758, 5), 126},
  {IR(42234, 15), 613},
  {IR(31911, 13), 140},
  {IR(42206, 13), 611},
  {IR(29228, 6), 1},
  {IR(32008, 10), 151},
  {IR(33193, 12), 233},
  {IR(35753, 22), 362},
  {IR(35890, 22), 368},
  {IR(36040, 25), 377},
  {IR(37614, 28), 441},
  {IR(35775, 30), 363},
  {IR(35857, 33), 367},
  {IR(35912, 19), 369},
  {IR(35832, 25), 366},
  {IR(35734, 19), 361},
  {IR(16099, 32), 848},
  {IR(30539, 9), 52},
  {IR(14087, 25), 743},
  {IR(30896, 21), 69},
  {IR(30917, 20), 70},
  {IR(30937, 14), 71},
  {IR(31819, 18), 133},
  {IR(31783, 18), 131},
  {IR(42467, 14), 625},
  {IR(38919, 33), 491},
  {IR(33418, 10), 250},
  {IR(49720, 24), 805},
  {IR(49744, 23), 805},
}};







static const std::array<ExtInstDesc, 500> kExtInstByValue{{
    {0,IR(811, 2),IR(51816, 5),IR(1, 0)},
    {0,IR(765, 9),IR(51821, 8),IR(1, 0)},
    {1,IR(765, 9),IR(51829, 13),IR(1, 0)},
    {2,IR(471, 8),IR(51842, 8),IR(1, 0)},
    {0,IR(0, 0),IR(51850, 14),IR(1, 0)},
    {1,IR(52, 3),IR(51864, 21),IR(1, 0)},
    {2,IR(813, 3),IR(51937, 15),IR(1, 0)},
    {3,IR(816, 3),IR(51986, 17),IR(1, 0)},
    {4,IR(819, 2),IR(52041, 19),IR(1, 0)},
    {5,IR(168, 2),IR(52060, 15),IR(1, 0)},
    {6,IR(261, 2),IR(52075, 16),IR(1, 0)},
    {7,IR(821, 6),IR(52091, 13),IR(1, 0)},
    {8,IR(168, 2),IR(52104, 18),IR(1, 0)},
    {9,IR(827, 9),IR(52122, 14),IR(1, 0)},
    {10,IR(836, 9),IR(52174, 19),IR(1, 0)},
    {11,IR(845, 10),IR(52193, 16),IR(1, 0)},
    {12,IR(855, 5),IR(52209, 21),IR(1, 0)},
    {13,IR(36, 2),IR(52230, 21),IR(1, 0)},
    {14,IR(168, 2),IR(52251, 18),IR(1, 0)},
    {15,IR(860, 6),IR(52269, 27),IR(1, 0)},
    {16,IR(866, 5),IR(52296, 35),IR(1, 0)},
    {17,IR(871, 5),IR(52331, 31),IR(1, 0)},
    {18,IR(845, 10),IR(52362, 20),IR(1, 0)},
    {19,IR(876, 8),IR(52382, 25),IR(1, 0)},
    {20,IR(884, 11),IR(52407, 14),IR(1, 0)},
    {21,IR(895, 5),IR(52421, 18),IR(1, 0)},
    {22,IR(900, 3),IR(52439, 31),IR(1, 0)},
    {23,IR(903, 2),IR(52470, 11),IR(1, 0)},
    {24,IR(0, 0),IR(52481, 13),IR(1, 0)},
    {25,IR(905, 3),IR(52494, 15),IR(1, 0)},
    {26,IR(908, 7),IR(52551, 19),IR(1, 0)},
    {27,IR(36, 2),IR(52570, 21),IR(1, 0)},
    {28,IR(33, 3),IR(52591, 13),IR(1, 0)},
    {29,IR(375, 3),IR(52604, 11),IR(1, 0)},
    {30,IR(915, 2),IR(52648, 15),IR(1, 0)},
    {31,IR(747, 1),IR(52663, 16),IR(1, 0)},
    {32,IR(917, 4),IR(52679, 14),IR(1, 0)},
    {33,IR(900, 3),IR(52693, 16),IR(1, 0)},
    {1,IR(10, 1),IR(52709, 6),IR(1, 0)},
    {2,IR(10, 1),IR(52715, 10),IR(1, 0)},
    {3,IR(10, 1),IR(52725, 6),IR(1, 0)},
    {4,IR(10, 1),IR(52731, 5),IR(1, 0)},
    {5,IR(10, 1),IR(52736, 5),IR(1, 0)},
    {6,IR(10, 1),IR(52741, 6),IR(1, 0)},
    {7,IR(10, 1),IR(52747, 6),IR(1, 0)},
    {8,IR(10, 1),IR(52753, 6),IR(1, 0)},
    {9,IR(10, 1),IR(52759, 5),IR(1, 0)},
    {10,IR(10, 1),IR(52764, 6),IR(1, 0)},
    {11,IR(10, 1),IR(52770, 8),IR(1, 0)},
    {12,IR(10, 1),IR(52778, 8),IR(1, 0)},
    {13,IR(10, 1),IR(52786, 4),IR(1, 0)},
    {14,IR(10, 1),IR(52790, 4),IR(1, 0)},
    {15,IR(10, 1),IR(52794, 4),IR(1, 0)},
    {16,IR(10, 1),IR(52798, 5),IR(1, 0)},
    {17,IR(10, 1),IR(52803, 5),IR(1, 0)},
    {18,IR(10, 1),IR(52808, 5),IR(1, 0)},
    {19,IR(10, 1),IR(52813, 5),IR(1, 0)},
    {20,IR(10, 1),IR(52818, 5),IR(1, 0)},
    {21,IR(10, 1),IR(52823, 5),IR(1, 0)},
    {22,IR(10, 1),IR(52828, 6),IR(1, 0)},
    {23,IR(10, 1),IR(52834, 6),IR(1, 0)},
    {24,IR(10, 1),IR(52840, 6),IR(1, 0)},
    {25,IR(36, 2),IR(52846, 6),IR(1, 0)},
    {26,IR(36, 2),IR(52852, 4),IR(1, 0)},
    {27,IR(10, 1),IR(52856, 4),IR(1, 0)},
    {28,IR(10, 1),IR(52860, 4),IR(1, 0)},
    {29,IR(10, 1),IR(52864, 5),IR(1, 0)},
    {30,IR(10, 1),IR(52869, 5),IR(1, 0)},
    {31,IR(10, 1),IR(52874, 5),IR(1, 0)},
    {32,IR(10, 1),IR(52879, 12),IR(1, 0)},
    {33,IR(10, 1),IR(52891, 12),IR(1, 0)},
    {34,IR(10, 1),IR(52903, 14),IR(1, 0)},
    {35,IR(36, 2),IR(52917, 5),IR(1, 0)},
    {36,IR(10, 1),IR(52922, 11),IR(1, 0)},
    {37,IR(36, 2),IR(52933, 5),IR(1, 0)},
    {38,IR(36, 2),IR(52938, 5),IR(1, 0)},
    {39,IR(36, 2),IR(52943, 5),IR(1, 0)},
    {40,IR(36, 2),IR(52948, 5),IR(1, 0)},
    {41,IR(36, 2),IR(52953, 5),IR(1, 0)},
    {42,IR(36, 2),IR(52958, 5),IR(1, 0)},
    {43,IR(33, 3),IR(52963, 7),IR(1, 0)},
    {44,IR(33, 3),IR(52970, 7),IR(1, 0)},
    {45,IR(33, 3),IR(52977, 7),IR(1, 0)},
    {46,IR(33, 3),IR(52984, 5),IR(1, 0)},
    {47,IR(33, 3),IR(52989, 5),IR(1, 0)},
    {48,IR(36, 2),IR(52994, 5),IR(1, 0)},
    {49,IR(33, 3),IR(52999, 11),IR(1, 0)},
    {50,IR(33, 3),IR(53010, 4),IR(1, 0)},
    {51,IR(36, 2),IR(53014, 6),IR(1, 0)},
    {52,IR(10, 1),IR(53020, 12),IR(1, 0)},
    {53,IR(36, 2),IR(53032, 6),IR(1, 0)},
    {54,IR(10, 1),IR(53038, 13),IR(1, 0)},
    {55,IR(10, 1),IR(53051, 13),IR(1, 0)},
    {56,IR(10, 1),IR(53064, 14),IR(1, 0)},
    {57,IR(10, 1),IR(53078, 14),IR(1, 0)},
    {58,IR(10, 1),IR(53092, 13),IR(1, 0)},
    {59,IR(10, 1),IR(53105, 15),IR(340, 1)},
    {60,IR(10, 1),IR(53120, 16),IR(1, 0)},
    {61,IR(10, 1),IR(53136, 16),IR(1, 0)},
    {62,IR(10, 1),IR(53152, 15),IR(1, 0)},
    {63,IR(10, 1),IR(53167, 15),IR(1, 0)},
    {64,IR(10, 1),IR(53182, 15),IR(1, 0)},
    {65,IR(10, 1),IR(53197, 17),IR(340, 1)},
    {66,IR(10, 1),IR(53214, 7),IR(1, 0)},
    {67,IR(36, 2),IR(53221, 9),IR(1, 0)},
    {68,IR(36, 2),IR(53230, 6),IR(1, 0)},
    {69,IR(10, 1),IR(53236, 10),IR(1, 0)},
    {70,IR(33, 3),IR(53246, 12),IR(1, 0)},
    {71,IR(36, 2),IR(53258, 8),IR(1, 0)},
    {72,IR(33, 3),IR(53266, 8),IR(1, 0)},
    {73,IR(10, 1),IR(53274, 9),IR(1, 0)},
    {74,IR(10, 1),IR(53283, 9),IR(1, 0)},
    {75,IR(10, 1),IR(53292, 9),IR(1, 0)},
    {76,IR(10, 1),IR(53301, 22),IR(341, 1)},
    {77,IR(36, 2),IR(53323, 20),IR(341, 1)},
    {78,IR(36, 2),IR(53343, 20),IR(341, 1)},
    {79,IR(36, 2),IR(53363, 5),IR(1, 0)},
    {80,IR(36, 2),IR(53368, 5),IR(1, 0)},
    {81,IR(33, 3),IR(53373, 7),IR(1, 0)},
    {1,IR(399, 5),IR(5827, 7),IR(1, 0)},
    {2,IR(921, 5),IR(53380, 13),IR(1, 0)},
    {3,IR(593, 5),IR(53393, 22),IR(1, 0)},
    {4,IR(593, 5),IR(53415, 16),IR(1, 0)},
    {5,IR(557, 7),IR(53431, 25),IR(1, 0)},
    {6,IR(557, 7),IR(53456, 19),IR(1, 0)},
    {7,IR(593, 5),IR(53475, 24),IR(1, 0)},
    {8,IR(593, 5),IR(53499, 21),IR(1, 0)},
    {9,IR(593, 5),IR(53520, 21),IR(1, 0)},
    {10,IR(593, 5),IR(53541, 16),IR(1, 0)},
    {11,IR(593, 5),IR(53557, 18),IR(1, 0)},
    {12,IR(33, 3),IR(53575, 26),IR(1, 0)},
    {13,IR(33, 3),IR(53601, 25),IR(1, 0)},
    {14,IR(10, 1),IR(53626, 20),IR(1, 0)},
    {15,IR(36, 2),IR(53646, 25),IR(1, 0)},
    {16,IR(36, 2),IR(53671, 30),IR(1, 0)},
    {17,IR(36, 2),IR(53701, 23),IR(1, 0)},
    {18,IR(36, 2),IR(53724, 25),IR(1, 0)},
    {19,IR(36, 2),IR(53749, 26),IR(1, 0)},
    {20,IR(36, 2),IR(53775, 30),IR(1, 0)},
    {21,IR(33, 3),IR(53805, 26),IR(1, 0)},
    {22,IR(33, 3),IR(53831, 20),IR(1, 0)},
    {23,IR(33, 3),IR(9301, 15),IR(1, 0)},
    {24,IR(296, 4),IR(53851, 30),IR(1, 0)},
    {25,IR(10, 1),IR(53881, 28),IR(1, 0)},
    {26,IR(593, 5),IR(53909, 28),IR(1, 0)},
    {27,IR(557, 7),IR(53937, 23),IR(1, 0)},
    {28,IR(33, 3),IR(53960, 35),IR(1, 0)},
    {29,IR(33, 3),IR(53995, 38),IR(1, 0)},
    {30,IR(296, 4),IR(54033, 42),IR(1, 0)},
    {31,IR(296, 4),IR(54075, 45),IR(1, 0)},
    {32,IR(535, 6),IR(54120, 37),IR(1, 0)},
    {33,IR(535, 6),IR(54157, 40),IR(1, 0)},
    {34,IR(593, 5),IR(54197, 27),IR(1, 0)},
    {35,IR(593, 5),IR(54224, 27),IR(1, 0)},
    {36,IR(33, 3),IR(54251, 32),IR(1, 0)},
    {37,IR(33, 3),IR(54283, 40),IR(1, 0)},
    {38,IR(375, 3),IR(54323, 11),IR(1, 0)},
    {39,IR(33, 3),IR(54334, 26),IR(1, 0)},
    {40,IR(33, 3),IR(54360, 32),IR(1, 0)},
    {41,IR(296, 4),IR(54392, 34),IR(1, 0)},
    {42,IR(36, 2),IR(54426, 22),IR(1, 0)},
    {1,IR(36, 2),IR(54448, 11),IR(1, 0)},
    {2,IR(375, 3),IR(52648, 15),IR(1, 0)},
    {3,IR(926, 3),IR(54459, 12),IR(1, 0)},
    {0,IR(0, 0),IR(51850, 14),IR(1, 0)},
    {1,IR(296, 4),IR(51864, 21),IR(1, 0)},
    {2,IR(593, 5),IR(51937, 15),IR(1, 0)},
    {3,IR(33, 3),IR(51986, 17),IR(1, 0)},
    {4,IR(36, 2),IR(52041, 19),IR(1, 0)},
    {5,IR(168, 2),IR(52060, 15),IR(1, 0)},
    {6,IR(36, 2),IR(52075, 16),IR(1, 0)},
    {7,IR(535, 6),IR(52091, 13),IR(1, 0)},
    {8,IR(375, 3),IR(52104, 18),IR(1, 0)},
    {9,IR(929, 9),IR(52122, 14),IR(1, 0)},
    {10,IR(938, 10),IR(52174, 19),IR(1, 0)},
    {11,IR(948, 9),IR(52193, 16),IR(1, 0)},
    {12,IR(296, 4),IR(52209, 21),IR(1, 0)},
    {13,IR(36, 2),IR(52230, 21),IR(1, 0)},
    {14,IR(168, 2),IR(52251, 18),IR(1, 0)},
    {15,IR(535, 6),IR(52269, 27),IR(1, 0)},
    {16,IR(675, 5),IR(52296, 35),IR(1, 0)},
    {17,IR(957, 5),IR(52331, 31),IR(1, 0)},
    {18,IR(962, 10),IR(52362, 20),IR(1, 0)},
    {19,IR(471, 8),IR(52382, 25),IR(1, 0)},
    {20,IR(962, 10),IR(52407, 14),IR(1, 0)},
    {21,IR(593, 5),IR(52421, 18),IR(1, 0)},
    {22,IR(33, 3),IR(52439, 31),IR(1, 0)},
    {23,IR(903, 2),IR(52470, 11),IR(1, 0)},
    {24,IR(0, 0),IR(52481, 13),IR(1, 0)},
    {25,IR(926, 3),IR(52494, 15),IR(1, 0)},
    {26,IR(972, 8),IR(52551, 19),IR(1, 0)},
    {27,IR(36, 2),IR(52570, 21),IR(1, 0)},
    {28,IR(980, 4),IR(52591, 13),IR(1, 0)},
    {29,IR(980, 4),IR(52604, 11),IR(1, 0)},
    {30,IR(168, 2),IR(52648, 15),IR(1, 0)},
    {31,IR(747, 1),IR(52663, 16),IR(1, 0)},
    {32,IR(580, 4),IR(52679, 14),IR(1, 0)},
    {33,IR(33, 3),IR(52693, 16),IR(1, 0)},
    {34,IR(516, 7),IR(54471, 20),IR(1, 0)},
    {35,IR(903, 2),IR(54491, 12),IR(1, 0)},
    {101,IR(36, 2),IR(54503, 24),IR(1, 0)},
    {102,IR(10, 1),IR(54527, 21),IR(1, 0)},
    {103,IR(675, 5),IR(54548, 10),IR(1, 0)},
    {104,IR(0, 0),IR(54558, 12),IR(1, 0)},
    {105,IR(36, 2),IR(54570, 21),IR(1, 0)},
    {106,IR(10, 1),IR(54591, 17),IR(1, 0)},
    {107,IR(296, 4),IR(54608, 16),IR(1, 0)},
    {108,IR(33, 3),IR(54624, 16),IR(1, 0)},
    {109,IR(36, 2),IR(54640, 21),IR(1, 0)},
    {110,IR(675, 5),IR(54661, 30),IR(1, 0)},
    {1,IR(765, 9),IR(54691, 14),IR(1, 0)},
    {2,IR(10, 1),IR(54705, 13),IR(1, 0)},
    {3,IR(10, 1),IR(54718, 12),IR(1, 0)},
    {4,IR(296, 4),IR(54730, 14),IR(1, 0)},
    {5,IR(33, 3),IR(54744, 23),IR(1, 0)},
    {6,IR(984, 15),IR(54767, 20),IR(1, 0)},
    {7,IR(999, 33),IR(54787, 19),IR(1, 0)},
    {8,IR(1032, 19),IR(54806, 21),IR(1, 0)},
    {0,IR(0, 0),IR(51850, 14),IR(1, 0)},
    {1,IR(1051, 4),IR(51864, 21),IR(1, 0)},
    {2,IR(1055, 3),IR(51937, 15),IR(1, 0)},
    {3,IR(1058, 3),IR(51986, 17),IR(1, 0)},
    {4,IR(1061, 2),IR(52041, 19),IR(1, 0)},
    {5,IR(168, 2),IR(52060, 15),IR(1, 0)},
    {6,IR(261, 2),IR(52075, 16),IR(1, 0)},
    {7,IR(821, 6),IR(52091, 13),IR(1, 0)},
    {8,IR(1063, 3),IR(52104, 18),IR(1, 0)},
    {9,IR(1066, 9),IR(52122, 14),IR(1, 0)},
    {10,IR(1075, 10),IR(52174, 19),IR(1, 0)},
    {11,IR(1085, 10),IR(52193, 16),IR(1, 0)},
    {12,IR(1095, 5),IR(52209, 21),IR(1, 0)},
    {13,IR(36, 2),IR(52230, 21),IR(1, 0)},
    {14,IR(168, 2),IR(52251, 18),IR(1, 0)},
    {15,IR(860, 6),IR(52269, 27),IR(1, 0)},
    {16,IR(866, 5),IR(52296, 35),IR(1, 0)},
    {17,IR(871, 5),IR(52331, 31),IR(1, 0)},
    {18,IR(1085, 10),IR(52362, 20),IR(1, 0)},
    {19,IR(1100, 8),IR(52382, 25),IR(1, 0)},
    {20,IR(1108, 11),IR(52407, 14),IR(1, 0)},
    {21,IR(895, 5),IR(52421, 18),IR(1, 0)},
    {22,IR(900, 3),IR(52439, 31),IR(1, 0)},
    {23,IR(903, 2),IR(52470, 11),IR(1, 0)},
    {24,IR(0, 0),IR(52481, 13),IR(1, 0)},
    {25,IR(905, 3),IR(52494, 15),IR(1, 0)},
    {26,IR(1119, 8),IR(52551, 19),IR(1, 0)},
    {27,IR(36, 2),IR(52570, 21),IR(1, 0)},
    {28,IR(33, 3),IR(52591, 13),IR(1, 0)},
    {29,IR(980, 4),IR(52604, 11),IR(1, 0)},
    {30,IR(1127, 2),IR(52648, 15),IR(1, 0)},
    {31,IR(747, 1),IR(52663, 16),IR(1, 0)},
    {32,IR(917, 4),IR(52679, 14),IR(1, 0)},
    {33,IR(900, 3),IR(52693, 16),IR(1, 0)},
    {34,IR(1129, 7),IR(54471, 20),IR(1, 0)},
    {35,IR(903, 2),IR(54491, 12),IR(1, 0)},
    {36,IR(1136, 8),IR(55127, 17),IR(1, 0)},
    {0,IR(10, 1),IR(55144, 5),IR(1, 0)},
    {1,IR(10, 1),IR(55149, 6),IR(1, 0)},
    {2,IR(10, 1),IR(55155, 7),IR(1, 0)},
    {3,IR(10, 1),IR(55162, 5),IR(1, 0)},
    {4,IR(10, 1),IR(55167, 6),IR(1, 0)},
    {5,IR(10, 1),IR(55173, 7),IR(1, 0)},
    {6,IR(10, 1),IR(55180, 5),IR(1, 0)},
    {7,IR(36, 2),IR(55185, 6),IR(1, 0)},
    {8,IR(10, 1),IR(55191, 6),IR(1, 0)},
    {9,IR(10, 1),IR(55197, 7),IR(1, 0)},
    {10,IR(36, 2),IR(55204, 8),IR(1, 0)},
    {11,IR(10, 1),IR(55212, 5),IR(1, 0)},
    {12,IR(10, 1),IR(55217, 5),IR(1, 0)},
    {13,IR(36, 2),IR(55222, 9),IR(1, 0)},
    {14,IR(10, 1),IR(55231, 4),IR(1, 0)},
    {15,IR(10, 1),IR(55235, 5),IR(1, 0)},
    {16,IR(10, 1),IR(55240, 6),IR(1, 0)},
    {17,IR(10, 1),IR(55246, 5),IR(1, 0)},
    {18,IR(10, 1),IR(55251, 4),IR(1, 0)},
    {19,IR(10, 1),IR(55255, 4),IR(1, 0)},
    {20,IR(10, 1),IR(55259, 5),IR(1, 0)},
    {21,IR(10, 1),IR(55264, 6),IR(1, 0)},
    {22,IR(10, 1),IR(55270, 6),IR(1, 0)},
    {23,IR(10, 1),IR(55276, 5),IR(1, 0)},
    {24,IR(36, 2),IR(55281, 5),IR(1, 0)},
    {25,IR(10, 1),IR(55286, 6),IR(1, 0)},
    {26,IR(33, 3),IR(55292, 4),IR(1, 0)},
    {27,IR(36, 2),IR(55296, 5),IR(1, 0)},
    {28,IR(36, 2),IR(55301, 5),IR(1, 0)},
    {29,IR(36, 2),IR(55306, 5),IR(1, 0)},
    {30,IR(36, 2),IR(55311, 6),IR(1, 0)},
    {31,IR(36, 2),IR(55317, 6),IR(1, 0)},
    {32,IR(36, 2),IR(55323, 6),IR(1, 0)},
    {33,IR(10, 1),IR(55329, 6),IR(1, 0)},
    {34,IR(36, 2),IR(55335, 6),IR(1, 0)},
    {35,IR(10, 1),IR(55341, 7),IR(1, 0)},
    {36,IR(36, 2),IR(55348, 9),IR(1, 0)},
    {37,IR(10, 1),IR(55357, 4),IR(1, 0)},
    {38,IR(10, 1),IR(55361, 5),IR(1, 0)},
    {39,IR(10, 1),IR(55366, 6),IR(1, 0)},
    {40,IR(10, 1),IR(55372, 6),IR(1, 0)},
    {41,IR(10, 1),IR(55378, 5),IR(1, 0)},
    {42,IR(33, 3),IR(55383, 4),IR(1, 0)},
    {43,IR(36, 2),IR(55387, 7),IR(1, 0)},
    {44,IR(36, 2),IR(55394, 7),IR(1, 0)},
    {45,IR(36, 2),IR(55401, 5),IR(1, 0)},
    {46,IR(10, 1),IR(55406, 4),IR(1, 0)},
    {47,IR(36, 2),IR(55410, 10),IR(1, 0)},
    {48,IR(36, 2),IR(55420, 4),IR(1, 0)},
    {49,IR(36, 2),IR(55424, 5),IR(1, 0)},
    {50,IR(36, 2),IR(55429, 5),IR(1, 0)},
    {51,IR(36, 2),IR(55434, 10),IR(1, 0)},
    {52,IR(33, 3),IR(55444, 7),IR(1, 0)},
    {53,IR(10, 1),IR(55451, 5),IR(1, 0)},
    {54,IR(36, 2),IR(55456, 6),IR(1, 0)},
    {55,IR(10, 1),IR(55462, 6),IR(1, 0)},
    {56,IR(10, 1),IR(55468, 6),IR(1, 0)},
    {57,IR(10, 1),IR(55474, 4),IR(1, 0)},
    {58,IR(36, 2),IR(55478, 7),IR(1, 0)},
    {59,IR(10, 1),IR(55485, 5),IR(1, 0)},
    {60,IR(10, 1),IR(55490, 6),IR(1, 0)},
    {61,IR(10, 1),IR(55496, 5),IR(1, 0)},
    {62,IR(10, 1),IR(55501, 4),IR(1, 0)},
    {63,IR(10, 1),IR(55505, 5),IR(1, 0)},
    {64,IR(10, 1),IR(55510, 6),IR(1, 0)},
    {65,IR(10, 1),IR(55516, 7),IR(1, 0)},
    {66,IR(10, 1),IR(55523, 6),IR(1, 0)},
    {67,IR(10, 1),IR(55529, 9),IR(1, 0)},
    {68,IR(36, 2),IR(55538, 12),IR(1, 0)},
    {69,IR(10, 1),IR(55550, 9),IR(1, 0)},
    {70,IR(10, 1),IR(55559, 10),IR(1, 0)},
    {71,IR(10, 1),IR(55569, 11),IR(1, 0)},
    {72,IR(10, 1),IR(55580, 9),IR(1, 0)},
    {73,IR(10, 1),IR(55589, 10),IR(1, 0)},
    {74,IR(10, 1),IR(55599, 11),IR(1, 0)},
    {75,IR(36, 2),IR(55610, 10),IR(1, 0)},
    {76,IR(10, 1),IR(55620, 11),IR(1, 0)},
    {77,IR(10, 1),IR(55631, 11),IR(1, 0)},
    {78,IR(10, 1),IR(55642, 9),IR(1, 0)},
    {79,IR(10, 1),IR(55651, 10),IR(1, 0)},
    {80,IR(10, 1),IR(55661, 9),IR(1, 0)},
    {81,IR(10, 1),IR(55670, 11),IR(1, 0)},
    {82,IR(36, 2),IR(55681, 14),IR(1, 0)},
    {83,IR(10, 1),IR(55695, 11),IR(1, 0)},
    {84,IR(10, 1),IR(55706, 12),IR(1, 0)},
    {85,IR(10, 1),IR(55718, 13),IR(1, 0)},
    {86,IR(10, 1),IR(55731, 11),IR(1, 0)},
    {87,IR(10, 1),IR(55742, 12),IR(1, 0)},
    {88,IR(10, 1),IR(55754, 13),IR(1, 0)},
    {89,IR(36, 2),IR(55767, 12),IR(1, 0)},
    {90,IR(10, 1),IR(55779, 13),IR(1, 0)},
    {91,IR(10, 1),IR(55792, 13),IR(1, 0)},
    {92,IR(10, 1),IR(55805, 11),IR(1, 0)},
    {93,IR(10, 1),IR(55816, 12),IR(1, 0)},
    {94,IR(10, 1),IR(55828, 11),IR(1, 0)},
    {95,IR(33, 3),IR(55839, 7),IR(1, 0)},
    {96,IR(10, 1),IR(55846, 8),IR(1, 0)},
    {97,IR(36, 2),IR(55854, 12),IR(1, 0)},
    {98,IR(36, 2),IR(55866, 12),IR(1, 0)},
    {99,IR(33, 3),IR(55878, 4),IR(1, 0)},
    {100,IR(10, 1),IR(55882, 8),IR(1, 0)},
    {101,IR(36, 2),IR(55890, 5),IR(1, 0)},
    {102,IR(33, 3),IR(55895, 11),IR(1, 0)},
    {103,IR(10, 1),IR(55906, 5),IR(1, 0)},
    {104,IR(36, 2),IR(55911, 6),IR(1, 0)},
    {105,IR(36, 2),IR(55917, 9),IR(1, 0)},
    {106,IR(10, 1),IR(55926, 7),IR(1, 0)},
    {107,IR(10, 1),IR(55933, 10),IR(1, 0)},
    {108,IR(36, 2),IR(55943, 14),IR(1, 0)},
    {109,IR(10, 1),IR(55957, 12),IR(1, 0)},
    {110,IR(10, 1),IR(55969, 15),IR(1, 0)},
    {141,IR(10, 1),IR(55984, 6),IR(1, 0)},
    {142,IR(36, 2),IR(55990, 11),IR(1, 0)},
    {143,IR(36, 2),IR(56001, 10),IR(1, 0)},
    {144,IR(36, 2),IR(56011, 10),IR(1, 0)},
    {145,IR(36, 2),IR(56021, 7),IR(1, 0)},
    {146,IR(36, 2),IR(56028, 7),IR(1, 0)},
    {147,IR(36, 2),IR(56035, 8),IR(1, 0)},
    {148,IR(36, 2),IR(56043, 8),IR(1, 0)},
    {149,IR(33, 3),IR(56051, 8),IR(1, 0)},
    {150,IR(33, 3),IR(56059, 8),IR(1, 0)},
    {151,IR(10, 1),IR(56067, 4),IR(1, 0)},
    {152,IR(10, 1),IR(56071, 4),IR(1, 0)},
    {153,IR(33, 3),IR(56075, 9),IR(1, 0)},
    {154,IR(33, 3),IR(56084, 10),IR(1, 0)},
    {155,IR(33, 3),IR(56094, 10),IR(1, 0)},
    {156,IR(36, 2),IR(56104, 6),IR(1, 0)},
    {157,IR(36, 2),IR(56110, 6),IR(1, 0)},
    {158,IR(36, 2),IR(56116, 6),IR(1, 0)},
    {159,IR(36, 2),IR(56122, 6),IR(1, 0)},
    {160,IR(36, 2),IR(56128, 9),IR(1, 0)},
    {161,IR(36, 2),IR(56137, 7),IR(1, 0)},
    {162,IR(36, 2),IR(56144, 10),IR(1, 0)},
    {163,IR(36, 2),IR(56154, 10),IR(1, 0)},
    {164,IR(36, 2),IR(56164, 11),IR(1, 0)},
    {165,IR(36, 2),IR(56175, 11),IR(1, 0)},
    {166,IR(10, 1),IR(56186, 9),IR(1, 0)},
    {167,IR(33, 3),IR(56195, 8),IR(1, 0)},
    {168,IR(33, 3),IR(56203, 8),IR(1, 0)},
    {169,IR(36, 2),IR(56211, 8),IR(1, 0)},
    {170,IR(36, 2),IR(56219, 8),IR(1, 0)},
    {171,IR(1144, 3),IR(56227, 7),IR(1, 0)},
    {172,IR(33, 3),IR(56234, 8),IR(1, 0)},
    {173,IR(36, 2),IR(56242, 11),IR(1, 0)},
    {174,IR(1144, 3),IR(56253, 12),IR(1, 0)},
    {175,IR(33, 3),IR(56265, 12),IR(1, 0)},
    {176,IR(1147, 4),IR(56277, 14),IR(1, 0)},
    {177,IR(33, 3),IR(56291, 13),IR(1, 0)},
    {178,IR(1147, 4),IR(56304, 15),IR(1, 0)},
    {179,IR(1144, 3),IR(56319, 13),IR(1, 0)},
    {180,IR(33, 3),IR(56332, 14),IR(1, 0)},
    {181,IR(1147, 4),IR(56346, 16),IR(1, 0)},
    {182,IR(36, 2),IR(56362, 8),IR(1, 0)},
    {183,IR(33, 3),IR(56370, 9),IR(1, 0)},
    {184,IR(168, 2),IR(56379, 7),IR(1, 0)},
    {185,IR(36, 2),IR(56386, 9),IR(1, 0)},
    {186,IR(33, 3),IR(56395, 10),IR(1, 0)},
    {187,IR(33, 3),IR(56405, 7),IR(1, 0)},
    {201,IR(10, 1),IR(56412, 6),IR(1, 0)},
    {202,IR(36, 2),IR(56418, 11),IR(1, 0)},
    {203,IR(36, 2),IR(56429, 9),IR(1, 0)},
    {204,IR(33, 3),IR(56438, 9),IR(1, 0)},
    {1,IR(10, 1),IR(56447, 17),IR(1, 0)},
    {2,IR(10, 1),IR(56464, 17),IR(1, 0)},
    {3,IR(0, 0),IR(56481, 8),IR(1, 0)},
    {1,IR(36, 2),IR(56489, 22),IR(1, 0)},
    {2,IR(36, 2),IR(56511, 28),IR(1, 0)},
    {3,IR(33, 3),IR(56539, 19),IR(1, 0)},
    {4,IR(10, 1),IR(56558, 9),IR(1, 0)},
    {1,IR(36, 2),IR(56567, 23),IR(1, 0)},
    {1,IR(33, 3),IR(56590, 9),IR(1, 0)},
    {2,IR(33, 3),IR(56599, 9),IR(1, 0)},
    {3,IR(33, 3),IR(56608, 9),IR(1, 0)},
    {4,IR(33, 3),IR(56617, 9),IR(1, 0)},
    {5,IR(33, 3),IR(56626, 9),IR(1, 0)},
    {6,IR(33, 3),IR(56635, 9),IR(1, 0)},
    {7,IR(33, 3),IR(56644, 9),IR(1, 0)},
    {8,IR(33, 3),IR(56653, 9),IR(1, 0)},
    {9,IR(33, 3),IR(56662, 9),IR(1, 0)},
    {0,IR(33, 3),IR(56671, 7),IR(1, 0)},
    {1,IR(516, 7),IR(56678, 11),IR(1, 0)},
    {2,IR(755, 10),IR(56689, 7),IR(1, 0)},
    {3,IR(755, 10),IR(56696, 7),IR(1, 0)},
    {4,IR(755, 10),IR(56703, 17),IR(1, 0)},
    {5,IR(296, 4),IR(56720, 6),IR(1, 0)},
    {6,IR(296, 4),IR(56726, 7),IR(1, 0)},
    {7,IR(675, 5),IR(56733, 11),IR(1, 0)},
    {8,IR(36, 2),IR(56744, 7),IR(1, 0)},
    {9,IR(765, 9),IR(56751, 17),IR(1, 0)},
    {10,IR(296, 4),IR(56768, 6),IR(1, 0)},
    {11,IR(10, 1),IR(56774, 4),IR(1, 0)},
    {12,IR(10, 1),IR(56778, 8),IR(1, 0)},
    {13,IR(10, 1),IR(56786, 5),IR(1, 0)},
    {14,IR(36, 2),IR(56791, 4),IR(1, 0)},
    {15,IR(33, 3),IR(56795, 23),IR(1, 0)},
    {16,IR(36, 2),IR(56818, 12),IR(1, 0)},
    {17,IR(36, 2),IR(56830, 11),IR(1, 0)},
    {18,IR(36, 2),IR(56841, 12),IR(1, 0)},
    {19,IR(36, 2),IR(56853, 7),IR(1, 0)},
    {20,IR(36, 2),IR(56860, 12),IR(1, 0)},
    {21,IR(36, 2),IR(56872, 19),IR(1, 0)},
    {22,IR(36, 2),IR(56891, 20),IR(1, 0)},
    {23,IR(36, 2),IR(56911, 11),IR(1, 0)},
    {24,IR(36, 2),IR(56922, 12),IR(1, 0)},
    {25,IR(33, 3),IR(56934, 8),IR(1, 0)},
    {26,IR(33, 3),IR(56942, 8),IR(1, 0)},
    {27,IR(33, 3),IR(56950, 4),IR(1, 0)},
    {28,IR(36, 2),IR(56954, 4),IR(1, 0)},
    {29,IR(36, 2),IR(56958, 4),IR(1, 0)},
    {30,IR(36, 2),IR(56962, 6),IR(1, 0)},
    {31,IR(10, 1),IR(56968, 4),IR(1, 0)},
    {32,IR(10, 1),IR(56972, 12),IR(1, 0)},
    {33,IR(10, 1),IR(56984, 5),IR(1, 0)},
    {34,IR(10, 1),IR(56989, 4),IR(1, 0)},
    {35,IR(10, 1),IR(56993, 4),IR(1, 0)},
    {36,IR(10, 1),IR(56997, 4),IR(1, 0)},
    {37,IR(10, 1),IR(57001, 6),IR(1, 0)},
    {38,IR(10, 1),IR(57007, 4),IR(1, 0)},
    {39,IR(10, 1),IR(57011, 12),IR(1, 0)},
    {40,IR(33, 3),IR(57023, 7),IR(1, 0)},
    {41,IR(10, 1),IR(57030, 11),IR(1, 0)},
    {42,IR(10, 1),IR(57041, 6),IR(1, 0)},
    {43,IR(10, 1),IR(57047, 4),IR(1, 0)},
    {44,IR(33, 3),IR(57051, 7),IR(1, 0)},
    {45,IR(36, 2),IR(57058, 6),IR(1, 0)},
    {46,IR(36, 2),IR(57064, 8),IR(1, 0)},
    {47,IR(36, 2),IR(57072, 14),IR(1, 0)},
    {48,IR(36, 2),IR(57086, 11),IR(1, 0)},
    {49,IR(36, 2),IR(57097, 11),IR(1, 0)},
    {50,IR(33, 3),IR(57108, 11),IR(1, 0)},
    {51,IR(33, 3),IR(57119, 11),IR(1, 0)},
    {52,IR(36, 2),IR(57130, 15),IR(1, 0)},
    {53,IR(36, 2),IR(57145, 11),IR(1, 0)},
    {54,IR(168, 2),IR(57156, 7),IR(1, 0)},
    {55,IR(33, 3),IR(57163, 4),IR(1, 0)},
    {56,IR(36, 2),IR(57167, 8),IR(1, 0)},
    {57,IR(36, 2),IR(57175, 8),IR(1, 0)},
    {58,IR(33, 3),IR(57183, 6),IR(1, 0)},
    {59,IR(36, 2),IR(57189, 5),IR(1, 0)},
    {60,IR(36, 2),IR(57194, 10),IR(1, 0)},
    {61,IR(36, 2),IR(57204, 7),IR(1, 0)},
    {62,IR(33, 3),IR(57211, 8),IR(1, 0)},
    {63,IR(675, 5),IR(57219, 7),IR(1, 0)},
    {64,IR(10, 1),IR(57226, 5),IR(1, 0)},
    {65,IR(755, 10),IR(57231, 8),IR(1, 0)},
}};

IndexRange ExtInstByValueRangeForKind(spv_ext_inst_type_t type) {
  switch(type) {
    case SPV_EXT_INST_TYPE_ARM_EXPERIMENTAL_ML_OPERATIONS: return IR(0, 1);
    case SPV_EXT_INST_TYPE_ARM_MOTION_ENGINE_100: return IR(1, 3);
    case SPV_EXT_INST_TYPE_DEBUGINFO: return IR(4, 34);
    case SPV_EXT_INST_TYPE_GLSL_STD_450: return IR(38, 81);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_CLSPVREFLECTION: return IR(119, 42);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_GRAPH_DEBUGINFO: return IR(161, 3);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_SHADER_DEBUGINFO_100: return IR(164, 46);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_VKSPREFLECTION: return IR(210, 8);
    case SPV_EXT_INST_TYPE_OPENCL_DEBUGINFO_100: return IR(218, 37);
    case SPV_EXT_INST_TYPE_OPENCL_STD: return IR(255, 162);
    case SPV_EXT_INST_TYPE_SPV_AMD_GCN_SHADER: return IR(417, 3);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_BALLOT: return IR(420, 4);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER: return IR(424, 1);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_TRINARY_MINMAX: return IR(425, 9);
    case SPV_EXT_INST_TYPE_TOSA_001000_1: return IR(434, 66);
    default: break;
  }
  return IR(0,0);
}





static const std::array<NameIndex, 500> kExtInstNames{{
    {IR(51816, 5), 0},
    {IR(51821, 8), 1},
    {IR(51829, 13), 2},
    {IR(51842, 8), 3},
    {IR(51864, 21), 5},
    {IR(52591, 13), 32},
    {IR(52663, 16), 35},
    {IR(52407, 14), 24},
    {IR(52382, 25), 23},
    {IR(52362, 20), 22},
    {IR(51850, 14), 4},
    {IR(52494, 15), 29},
    {IR(52570, 21), 31},
    {IR(52421, 18), 25},
    {IR(52439, 31), 26},
    {IR(52551, 19), 30},
    {IR(52679, 14), 36},
    {IR(52693, 16), 37},
    {IR(52481, 13), 28},
    {IR(52648, 15), 34},
    {IR(52470, 11), 27},
    {IR(52060, 15), 9},
    {IR(51937, 15), 6},
    {IR(52174, 19), 14},
    {IR(52122, 14), 13},
    {IR(52104, 18), 12},
    {IR(52209, 21), 16},
    {IR(52193, 16), 15},
    {IR(51986, 17), 7},
    {IR(52230, 21), 17},
    {IR(52041, 19), 8},
    {IR(52251, 18), 18},
    {IR(52269, 27), 19},
    {IR(52331, 31), 21},
    {IR(52296, 35), 20},
    {IR(52075, 16), 10},
    {IR(52091, 13), 11},
    {IR(52604, 11), 33},
    {IR(52803, 5), 54},
    {IR(52834, 6), 60},
    {IR(52798, 5), 53},
    {IR(52828, 6), 59},
    {IR(52808, 5), 55},
    {IR(52846, 6), 62},
    {IR(52840, 6), 61},
    {IR(52759, 5), 46},
    {IR(52790, 4), 51},
    {IR(52818, 5), 57},
    {IR(53230, 6), 105},
    {IR(52778, 8), 49},
    {IR(52891, 12), 70},
    {IR(53221, 9), 104},
    {IR(52856, 4), 64},
    {IR(52864, 5), 66},
    {IR(52731, 5), 41},
    {IR(52963, 7), 80},
    {IR(52948, 5), 77},
    {IR(52933, 5), 74},
    {IR(52984, 5), 83},
    {IR(52741, 6), 43},
    {IR(53246, 12), 107},
    {IR(53274, 9), 110},
    {IR(53283, 9), 111},
    {IR(53292, 9), 112},
    {IR(52753, 6), 45},
    {IR(53010, 4), 87},
    {IR(52764, 6), 47},
    {IR(53014, 6), 88},
    {IR(53020, 12), 89},
    {IR(52989, 5), 84},
    {IR(53301, 22), 113},
    {IR(53343, 20), 115},
    {IR(53323, 20), 114},
    {IR(52879, 12), 69},
    {IR(53032, 6), 90},
    {IR(53214, 7), 103},
    {IR(52860, 4), 65},
    {IR(52869, 5), 67},
    {IR(52903, 14), 71},
    {IR(52917, 5), 72},
    {IR(52922, 11), 73},
    {IR(53373, 7), 118},
    {IR(53368, 5), 117},
    {IR(53363, 5), 116},
    {IR(53236, 10), 106},
    {IR(53105, 15), 96},
    {IR(53092, 13), 95},
    {IR(53064, 14), 93},
    {IR(53038, 13), 91},
    {IR(53078, 14), 94},
    {IR(53051, 13), 92},
    {IR(52852, 4), 63},
    {IR(52770, 8), 48},
    {IR(53258, 8), 108},
    {IR(53266, 8), 109},
    {IR(52709, 6), 38},
    {IR(52715, 10), 39},
    {IR(52736, 5), 42},
    {IR(52977, 7), 82},
    {IR(52958, 5), 79},
    {IR(52943, 5), 76},
    {IR(52747, 6), 44},
    {IR(52786, 4), 50},
    {IR(52813, 5), 56},
    {IR(52999, 11), 86},
    {IR(52874, 5), 68},
    {IR(52994, 5), 85},
    {IR(52794, 4), 52},
    {IR(52823, 5), 58},
    {IR(52725, 6), 40},
    {IR(52970, 7), 81},
    {IR(52953, 5), 78},
    {IR(52938, 5), 75},
    {IR(53197, 17), 102},
    {IR(53152, 15), 99},
    {IR(53120, 16), 97},
    {IR(53167, 15), 100},
    {IR(53136, 16), 98},
    {IR(53182, 15), 101},
    {IR(53380, 13), 120},
    {IR(53475, 24), 125},
    {IR(53431, 25), 123},
    {IR(53456, 19), 124},
    {IR(53909, 28), 144},
    {IR(53937, 23), 145},
    {IR(53499, 21), 126},
    {IR(53541, 16), 128},
    {IR(53393, 22), 121},
    {IR(53520, 21), 127},
    {IR(54197, 27), 152},
    {IR(53415, 16), 122},
    {IR(54224, 27), 153},
    {IR(53557, 18), 129},
    {IR(54251, 32), 154},
    {IR(53805, 26), 139},
    {IR(53831, 20), 140},
    {IR(54075, 45), 149},
    {IR(54157, 40), 151},
    {IR(54033, 42), 148},
    {IR(54120, 37), 150},
    {IR(5827, 7), 119},
    {IR(9301, 15), 141},
    {IR(54392, 34), 159},
    {IR(54360, 32), 158},
    {IR(54334, 26), 157},
    {IR(54323, 11), 156},
    {IR(54283, 40), 155},
    {IR(53995, 38), 147},
    {IR(53960, 35), 146},
    {IR(53851, 30), 142},
    {IR(53671, 30), 134},
    {IR(53646, 25), 133},
    {IR(53701, 23), 135},
    {IR(53749, 26), 137},
    {IR(53775, 30), 138},
    {IR(53724, 25), 136},
    {IR(53601, 25), 131},
    {IR(53881, 28), 143},
    {IR(53626, 20), 132},
    {IR(53575, 26), 130},
    {IR(54426, 22), 160},
    {IR(54448, 11), 161},
    {IR(52648, 15), 162},
    {IR(54459, 12), 163},
    {IR(54570, 21), 204},
    {IR(51864, 21), 165},
    {IR(52591, 13), 192},
    {IR(54608, 16), 206},
    {IR(52663, 16), 195},
    {IR(52407, 14), 184},
    {IR(52382, 25), 183},
    {IR(54503, 24), 200},
    {IR(52362, 20), 182},
    {IR(54471, 20), 198},
    {IR(51850, 14), 164},
    {IR(52494, 15), 189},
    {IR(52570, 21), 191},
    {IR(52421, 18), 185},
    {IR(52439, 31), 186},
    {IR(54548, 10), 202},
    {IR(52551, 19), 190},
    {IR(52679, 14), 196},
    {IR(52693, 16), 197},
    {IR(54558, 12), 203},
    {IR(52481, 13), 188},
    {IR(52648, 15), 194},
    {IR(52470, 11), 187},
    {IR(54491, 12), 199},
    {IR(54527, 21), 201},
    {IR(54591, 17), 205},
    {IR(52060, 15), 169},
    {IR(51937, 15), 166},
    {IR(52174, 19), 174},
    {IR(54661, 30), 209},
    {IR(52122, 14), 173},
    {IR(52104, 18), 172},
    {IR(52209, 21), 176},
    {IR(54624, 16), 207},
    {IR(52193, 16), 175},
    {IR(51986, 17), 167},
    {IR(52230, 21), 177},
    {IR(52041, 19), 168},
    {IR(52251, 18), 178},
    {IR(52269, 27), 179},
    {IR(52331, 31), 181},
    {IR(52296, 35), 180},
    {IR(52075, 16), 170},
    {IR(54640, 21), 208},
    {IR(52091, 13), 171},
    {IR(52604, 11), 193},
    {IR(54691, 14), 210},
    {IR(54767, 20), 215},
    {IR(54787, 19), 216},
    {IR(54806, 21), 217},
    {IR(54730, 14), 213},
    {IR(54744, 23), 214},
    {IR(54705, 13), 211},
    {IR(54718, 12), 212},
    {IR(51864, 21), 219},
    {IR(52591, 13), 246},
    {IR(52663, 16), 249},
    {IR(52407, 14), 238},
    {IR(52382, 25), 237},
    {IR(52362, 20), 236},
    {IR(54471, 20), 252},
    {IR(51850, 14), 218},
    {IR(52494, 15), 243},
    {IR(52570, 21), 245},
    {IR(52421, 18), 239},
    {IR(52439, 31), 240},
    {IR(52551, 19), 244},
    {IR(52679, 14), 250},
    {IR(52693, 16), 251},
    {IR(55127, 17), 254},
    {IR(52481, 13), 242},
    {IR(52648, 15), 248},
    {IR(52470, 11), 241},
    {IR(54491, 12), 253},
    {IR(52060, 15), 223},
    {IR(51937, 15), 220},
    {IR(52174, 19), 228},
    {IR(52122, 14), 227},
    {IR(52104, 18), 226},
    {IR(52209, 21), 230},
    {IR(52193, 16), 229},
    {IR(51986, 17), 221},
    {IR(52230, 21), 231},
    {IR(52041, 19), 222},
    {IR(52251, 18), 232},
    {IR(52269, 27), 233},
    {IR(52331, 31), 235},
    {IR(52296, 35), 234},
    {IR(52075, 16), 224},
    {IR(52091, 13), 225},
    {IR(52604, 11), 247},
    {IR(55144, 5), 255},
    {IR(55149, 6), 256},
    {IR(55155, 7), 257},
    {IR(55162, 5), 258},
    {IR(55167, 6), 259},
    {IR(55173, 7), 260},
    {IR(55180, 5), 261},
    {IR(55185, 6), 262},
    {IR(55204, 8), 265},
    {IR(55191, 6), 263},
    {IR(55197, 7), 264},
    {IR(56395, 10), 411},
    {IR(55212, 5), 266},
    {IR(55217, 5), 267},
    {IR(56067, 4), 376},
    {IR(55222, 9), 268},
    {IR(55231, 4), 269},
    {IR(55235, 5), 270},
    {IR(55240, 6), 271},
    {IR(55911, 6), 359},
    {IR(56071, 4), 377},
    {IR(55846, 8), 351},
    {IR(55917, 9), 360},
    {IR(55251, 4), 273},
    {IR(55246, 5), 272},
    {IR(55255, 4), 274},
    {IR(55264, 6), 276},
    {IR(55259, 5), 275},
    {IR(55270, 6), 277},
    {IR(55276, 5), 278},
    {IR(55943, 14), 363},
    {IR(55957, 12), 364},
    {IR(55969, 15), 365},
    {IR(55839, 7), 350},
    {IR(55281, 5), 279},
    {IR(55286, 6), 280},
    {IR(55292, 4), 281},
    {IR(55296, 5), 282},
    {IR(55854, 12), 352},
    {IR(55301, 5), 283},
    {IR(55866, 12), 353},
    {IR(55306, 5), 284},
    {IR(55311, 6), 285},
    {IR(55317, 6), 286},
    {IR(55529, 9), 322},
    {IR(55538, 12), 323},
    {IR(55550, 9), 324},
    {IR(55569, 11), 326},
    {IR(55559, 10), 325},
    {IR(55580, 9), 327},
    {IR(55599, 11), 329},
    {IR(55589, 10), 328},
    {IR(55610, 10), 330},
    {IR(55620, 11), 331},
    {IR(55631, 11), 332},
    {IR(55642, 9), 333},
    {IR(55651, 10), 334},
    {IR(55661, 9), 335},
    {IR(55323, 6), 287},
    {IR(55329, 6), 288},
    {IR(55335, 6), 289},
    {IR(55926, 7), 361},
    {IR(55341, 7), 290},
    {IR(55348, 9), 291},
    {IR(55357, 4), 292},
    {IR(55366, 6), 294},
    {IR(55372, 6), 295},
    {IR(55361, 5), 293},
    {IR(55378, 5), 296},
    {IR(55383, 4), 297},
    {IR(55387, 7), 298},
    {IR(55394, 7), 299},
    {IR(55878, 4), 354},
    {IR(55401, 5), 300},
    {IR(55406, 4), 301},
    {IR(55670, 11), 336},
    {IR(55681, 14), 337},
    {IR(55695, 11), 338},
    {IR(55718, 13), 340},
    {IR(55706, 12), 339},
    {IR(55731, 11), 341},
    {IR(55754, 13), 343},
    {IR(55742, 12), 342},
    {IR(55767, 12), 344},
    {IR(55779, 13), 345},
    {IR(55792, 13), 346},
    {IR(55805, 11), 347},
    {IR(55816, 12), 348},
    {IR(55828, 11), 349},
    {IR(55410, 10), 302},
    {IR(55933, 10), 362},
    {IR(56186, 9), 391},
    {IR(55420, 4), 303},
    {IR(55424, 5), 304},
    {IR(55429, 5), 305},
    {IR(56386, 9), 410},
    {IR(56379, 7), 409},
    {IR(55882, 8), 355},
    {IR(55434, 10), 306},
    {IR(55444, 7), 307},
    {IR(55451, 5), 308},
    {IR(55456, 6), 309},
    {IR(56137, 7), 386},
    {IR(55462, 6), 310},
    {IR(55468, 6), 311},
    {IR(55984, 6), 366},
    {IR(55990, 11), 367},
    {IR(56001, 10), 368},
    {IR(56051, 8), 374},
    {IR(56021, 7), 370},
    {IR(56195, 8), 392},
    {IR(56075, 9), 378},
    {IR(56094, 10), 380},
    {IR(56104, 6), 381},
    {IR(56116, 6), 383},
    {IR(56211, 8), 394},
    {IR(56128, 9), 385},
    {IR(56035, 8), 372},
    {IR(56144, 10), 387},
    {IR(56175, 11), 390},
    {IR(56405, 7), 412},
    {IR(56362, 8), 407},
    {IR(56370, 9), 408},
    {IR(55906, 5), 358},
    {IR(55474, 4), 312},
    {IR(55478, 7), 313},
    {IR(55485, 5), 314},
    {IR(55490, 6), 315},
    {IR(55895, 11), 357},
    {IR(55496, 5), 316},
    {IR(55890, 5), 356},
    {IR(55501, 4), 317},
    {IR(55505, 5), 318},
    {IR(55510, 6), 319},
    {IR(55516, 7), 320},
    {IR(55523, 6), 321},
    {IR(56412, 6), 413},
    {IR(56418, 11), 414},
    {IR(56011, 10), 369},
    {IR(56059, 8), 375},
    {IR(56028, 7), 371},
    {IR(56203, 8), 393},
    {IR(56438, 9), 416},
    {IR(56084, 10), 379},
    {IR(56110, 6), 382},
    {IR(56122, 6), 384},
    {IR(56219, 8), 395},
    {IR(56429, 9), 415},
    {IR(56043, 8), 373},
    {IR(56154, 10), 388},
    {IR(56164, 11), 389},
    {IR(56242, 11), 398},
    {IR(56253, 12), 399},
    {IR(56319, 13), 404},
    {IR(56227, 7), 396},
    {IR(56265, 12), 400},
    {IR(56277, 14), 401},
    {IR(56291, 13), 402},
    {IR(56304, 15), 403},
    {IR(56332, 14), 405},
    {IR(56346, 16), 406},
    {IR(56234, 8), 397},
    {IR(56464, 17), 418},
    {IR(56447, 17), 417},
    {IR(56481, 8), 419},
    {IR(56558, 9), 423},
    {IR(56489, 22), 420},
    {IR(56511, 28), 421},
    {IR(56539, 19), 422},
    {IR(56567, 23), 424},
    {IR(56617, 9), 428},
    {IR(56644, 9), 431},
    {IR(56590, 9), 425},
    {IR(56635, 9), 430},
    {IR(56662, 9), 433},
    {IR(56608, 9), 427},
    {IR(56626, 9), 429},
    {IR(56653, 9), 432},
    {IR(56599, 9), 426},
    {IR(56968, 4), 465},
    {IR(56791, 4), 448},
    {IR(56671, 7), 434},
    {IR(56795, 23), 449},
    {IR(56678, 11), 435},
    {IR(56818, 12), 450},
    {IR(56972, 12), 466},
    {IR(56830, 11), 451},
    {IR(56841, 12), 452},
    {IR(57226, 5), 498},
    {IR(56984, 5), 467},
    {IR(56768, 6), 444},
    {IR(56989, 4), 468},
    {IR(57156, 7), 488},
    {IR(56689, 7), 436},
    {IR(56696, 7), 437},
    {IR(56993, 4), 469},
    {IR(56703, 17), 438},
    {IR(57058, 6), 479},
    {IR(56774, 4), 445},
    {IR(56997, 4), 470},
    {IR(56720, 6), 439},
    {IR(57001, 6), 471},
    {IR(57204, 7), 495},
    {IR(57064, 8), 480},
    {IR(57072, 14), 481},
    {IR(56853, 7), 453},
    {IR(57007, 4), 472},
    {IR(56860, 12), 454},
    {IR(56872, 19), 455},
    {IR(57011, 12), 473},
    {IR(56911, 11), 457},
    {IR(56891, 20), 456},
    {IR(56922, 12), 458},
    {IR(56726, 7), 440},
    {IR(56934, 8), 459},
    {IR(56733, 11), 441},
    {IR(56942, 8), 460},
    {IR(56950, 4), 461},
    {IR(57023, 7), 474},
    {IR(57163, 4), 489},
    {IR(56954, 4), 462},
    {IR(57030, 11), 475},
    {IR(57086, 11), 482},
    {IR(57097, 11), 483},
    {IR(57108, 11), 484},
    {IR(57119, 11), 485},
    {IR(57130, 15), 486},
    {IR(57145, 11), 487},
    {IR(57231, 8), 499},
    {IR(57167, 8), 490},
    {IR(57219, 7), 497},
    {IR(57175, 8), 491},
    {IR(56744, 7), 442},
    {IR(57041, 6), 476},
    {IR(57211, 8), 496},
    {IR(57051, 7), 478},
    {IR(56778, 8), 446},
    {IR(57047, 4), 477},
    {IR(57183, 6), 492},
    {IR(56958, 4), 463},
    {IR(56962, 6), 464},
    {IR(56786, 5), 447},
    {IR(57189, 5), 493},
    {IR(57194, 10), 494},
    {IR(56751, 17), 443},
}};

IndexRange ExtInstNameRangeForKind(spv_ext_inst_type_t type) {
  switch(type) {
    case SPV_EXT_INST_TYPE_ARM_EXPERIMENTAL_ML_OPERATIONS: return IR(0, 1);
    case SPV_EXT_INST_TYPE_ARM_MOTION_ENGINE_100: return IR(1, 3);
    case SPV_EXT_INST_TYPE_DEBUGINFO: return IR(4, 34);
    case SPV_EXT_INST_TYPE_GLSL_STD_450: return IR(38, 81);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_CLSPVREFLECTION: return IR(119, 42);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_GRAPH_DEBUGINFO: return IR(161, 3);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_SHADER_DEBUGINFO_100: return IR(164, 46);
    case SPV_EXT_INST_TYPE_NONSEMANTIC_VKSPREFLECTION: return IR(210, 8);
    case SPV_EXT_INST_TYPE_OPENCL_DEBUGINFO_100: return IR(218, 37);
    case SPV_EXT_INST_TYPE_OPENCL_STD: return IR(255, 162);
    case SPV_EXT_INST_TYPE_SPV_AMD_GCN_SHADER: return IR(417, 3);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_BALLOT: return IR(420, 4);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER: return IR(424, 1);
    case SPV_EXT_INST_TYPE_SPV_AMD_SHADER_TRINARY_MINMAX: return IR(425, 9);
    case SPV_EXT_INST_TYPE_TOSA_001000_1: return IR(434, 66);
    default: break;
  }
  return IR(0,0);
}



static const char kStrings[] =
  "SPV_ALTERA_arbitrary_precision_fixed_point\0"
  "SPV_ALTERA_arbitrary_precision_floating_point\0"
  "SPV_ALTERA_arbitrary_precision_integers\0"
  "SPV_ALTERA_blocking_pipes\0"
  "SPV_ALTERA_fpga_argument_interfaces\0"
  "SPV_ALTERA_fpga_buffer_location\0"
  "SPV_ALTERA_fpga_cluster_attributes\0"
  "SPV_ALTERA_fpga_dsp_control\0"
  "SPV_ALTERA_fpga_invocation_pipelining_attributes\0"
  "SPV_ALTERA_fpga_latency_control\0"
  "SPV_ALTERA_fpga_loop_controls\0"
  "SPV_ALTERA_fpga_memory_accesses\0"
  "SPV_ALTERA_fpga_memory_attributes\0"
  "SPV_ALTERA_fpga_reg\0"
  "SPV_ALTERA_global_variable_fpga_decorations\0"
  "SPV_ALTERA_io_pipes\0"
  "SPV_ALTERA_loop_fuse\0"
  "SPV_ALTERA_runtime_aligned\0"
  "SPV_ALTERA_task_sequence\0"
  "SPV_ALTERA_usm_storage_classes\0"
  "SPV_AMDX_shader_enqueue\0"
  "SPV_AMD_gcn_shader\0"
  "SPV_AMD_gpu_shader_half_float\0"
  "SPV_AMD_gpu_shader_half_float_fetch\0"
  "SPV_AMD_gpu_shader_int16\0"
  "SPV_AMD_shader_ballot\0"
  "SPV_AMD_shader_early_and_late_fragment_tests\0"
  "SPV_AMD_shader_explicit_vertex_parameter\0"
  "SPV_AMD_shader_fragment_mask\0"
  "SPV_AMD_shader_image_load_store_lod\0"
  "SPV_AMD_shader_trinary_minmax\0"
  "SPV_AMD_texture_gather_bias_lod\0"
  "SPV_AMD_weak_linkage\0"
  "SPV_ARM_cooperative_matrix_layouts\0"
  "SPV_ARM_core_builtins\0"
  "SPV_ARM_graph\0"
  "SPV_ARM_tensors\0"
  "SPV_EXT_arithmetic_fence\0"
  "SPV_EXT_demote_to_helper_invocation\0"
  "SPV_EXT_descriptor_heap\0"
  "SPV_EXT_descriptor_indexing\0"
  "SPV_EXT_float8\0"
  "SPV_EXT_fragment_fully_covered\0"
  "SPV_EXT_fragment_invocation_density\0"
  "SPV_EXT_fragment_shader_interlock\0"
  "SPV_EXT_long_vector\0"
  "SPV_EXT_mesh_shader\0"
  "SPV_EXT_ocp_microscaling_types\0"
  "SPV_EXT_opacity_micromap\0"
  "SPV_EXT_optnone\0"
  "SPV_EXT_physical_storage_buffer\0"
  "SPV_EXT_relaxed_printf_string_address_space\0"
  "SPV_EXT_replicated_composites\0"
  "SPV_EXT_shader_64bit_indexing\0"
  "SPV_EXT_shader_atomic_float16_add\0"
  "SPV_EXT_shader_atomic_float_add\0"
  "SPV_EXT_shader_atomic_float_min_max\0"
  "SPV_EXT_shader_image_int64\0"
  "SPV_EXT_shader_invocation_reorder\0"
  "SPV_EXT_shader_stencil_export\0"
  "SPV_EXT_shader_subgroup_partitioned\0"
  "SPV_EXT_shader_tile_image\0"
  "SPV_EXT_shader_viewport_index_layer\0"
  "SPV_EXT_split_barrier\0"
  "SPV_GOOGLE_decorate_string\0"
  "SPV_GOOGLE_hlsl_functionality1\0"
  "SPV_GOOGLE_user_type\0"
  "SPV_INTEL_2d_block_io\0"
  "SPV_INTEL_arbitrary_precision_fixed_point\0"
  "SPV_INTEL_arbitrary_precision_floating_point\0"
  "SPV_INTEL_arbitrary_precision_integers\0"
  "SPV_INTEL_bfloat16_conversion\0"
  "SPV_INTEL_bindless_images\0"
  "SPV_INTEL_blocking_pipes\0"
  "SPV_INTEL_cache_controls\0"
  "SPV_INTEL_debug_module\0"
  "SPV_INTEL_device_side_avc_motion_estimation\0"
  "SPV_INTEL_float_controls2\0"
  "SPV_INTEL_fp_fast_math_mode\0"
  "SPV_INTEL_fp_max_error\0"
  "SPV_INTEL_fpga_argument_interfaces\0"
  "SPV_INTEL_fpga_buffer_location\0"
  "SPV_INTEL_fpga_cluster_attributes\0"
  "SPV_INTEL_fpga_dsp_control\0"
  "SPV_INTEL_fpga_invocation_pipelining_attributes\0"
  "SPV_INTEL_fpga_latency_control\0"
  "SPV_INTEL_fpga_loop_controls\0"
  "SPV_INTEL_fpga_memory_accesses\0"
  "SPV_INTEL_fpga_memory_attributes\0"
  "SPV_INTEL_fpga_reg\0"
  "SPV_INTEL_function_pointers\0"
  "SPV_INTEL_function_variants\0"
  "SPV_INTEL_global_variable_fpga_decorations\0"
  "SPV_INTEL_global_variable_host_access\0"
  "SPV_INTEL_inline_assembly\0"
  "SPV_INTEL_int4\0"
  "SPV_INTEL_io_pipes\0"
  "SPV_INTEL_kernel_attributes\0"
  "SPV_INTEL_long_composites\0"
  "SPV_INTEL_loop_fuse\0"
  "SPV_INTEL_masked_gather_scatter\0"
  "SPV_INTEL_maximum_registers\0"
  "SPV_INTEL_media_block_io\0"
  "SPV_INTEL_memory_access_aliasing\0"
  "SPV_INTEL_optnone\0"
  "SPV_INTEL_predicated_io\0"
  "SPV_INTEL_rounded_divide_sqrt\0"
  "SPV_INTEL_runtime_aligned\0"
  "SPV_INTEL_shader_integer_functions2\0"
  "SPV_INTEL_split_barrier\0"
  "SPV_INTEL_subgroup_buffer_prefetch\0"
  "SPV_INTEL_subgroup_matrix_multiply_accumulate\0"
  "SPV_INTEL_subgroups\0"
  "SPV_INTEL_task_sequence\0"
  "SPV_INTEL_tensor_float32_conversion\0"
  "SPV_INTEL_ternary_bitwise_function\0"
  "SPV_INTEL_unstructured_loop_controls\0"
  "SPV_INTEL_usm_storage_classes\0"
  "SPV_INTEL_variable_length_array\0"
  "SPV_INTEL_vector_compute\0"
  "SPV_KHR_16bit_storage\0"
  "SPV_KHR_8bit_storage\0"
  "SPV_KHR_abort\0"
  "SPV_KHR_bfloat16\0"
  "SPV_KHR_bit_instructions\0"
  "SPV_KHR_compute_shader_derivatives\0"
  "SPV_KHR_constant_data\0"
  "SPV_KHR_cooperative_matrix\0"
  "SPV_KHR_device_group\0"
  "SPV_KHR_expect_assume\0"
  "SPV_KHR_float_controls\0"
  "SPV_KHR_float_controls2\0"
  "SPV_KHR_fma\0"
  "SPV_KHR_fragment_shader_barycentric\0"
  "SPV_KHR_fragment_shading_rate\0"
  "SPV_KHR_integer_dot_product\0"
  "SPV_KHR_linkonce_odr\0"
  "SPV_KHR_maximal_reconvergence\0"
  "SPV_KHR_multiview\0"
  "SPV_KHR_no_integer_wrap_decoration\0"
  "SPV_KHR_non_semantic_info\0"
  "SPV_KHR_opacity_micromap\0"
  "SPV_KHR_physical_storage_buffer\0"
  "SPV_KHR_poison_freeze\0"
  "SPV_KHR_post_depth_coverage\0"
  "SPV_KHR_quad_control\0"
  "SPV_KHR_ray_cull_mask\0"
  "SPV_KHR_ray_query\0"
  "SPV_KHR_ray_tracing\0"
  "SPV_KHR_ray_tracing_position_fetch\0"
  "SPV_KHR_relaxed_extended_instruction\0"
  "SPV_KHR_shader_atomic_counter_ops\0"
  "SPV_KHR_shader_ballot\0"
  "SPV_KHR_shader_clock\0"
  "SPV_KHR_shader_draw_parameters\0"
  "SPV_KHR_storage_buffer_storage_class\0"
  "SPV_KHR_subgroup_rotate\0"
  "SPV_KHR_subgroup_uniform_control_flow\0"
  "SPV_KHR_subgroup_vote\0"
  "SPV_KHR_terminate_invocation\0"
  "SPV_KHR_uniform_group_instructions\0"
  "SPV_KHR_untyped_pointers\0"
  "SPV_KHR_variable_pointers\0"
  "SPV_KHR_vulkan_memory_model\0"
  "SPV_KHR_workgroup_memory_explicit_layout\0"
  "SPV_NVX_multiview_per_view_attributes\0"
  "SPV_NV_bindless_texture\0"
  "SPV_NV_cluster_acceleration_structure\0"
  "SPV_NV_compute_shader_derivatives\0"
  "SPV_NV_cooperative_matrix\0"
  "SPV_NV_cooperative_matrix2\0"
  "SPV_NV_cooperative_matrix_decode_vector\0"
  "SPV_NV_cooperative_vector\0"
  "SPV_NV_displacement_micromap\0"
  "SPV_NV_fragment_shader_barycentric\0"
  "SPV_NV_geometry_shader_passthrough\0"
  "SPV_NV_linear_swept_spheres\0"
  "SPV_NV_mesh_shader\0"
  "SPV_NV_push_constant_bank\0"
  "SPV_NV_raw_access_chains\0"
  "SPV_NV_ray_tracing\0"
  "SPV_NV_ray_tracing_motion_blur\0"
  "SPV_NV_sample_mask_override_coverage\0"
  "SPV_NV_shader_atomic_fp16_vector\0"
  "SPV_NV_shader_image_footprint\0"
  "SPV_NV_shader_invocation_reorder\0"
  "SPV_NV_shader_sm_builtins\0"
  "SPV_NV_shader_subgroup_partitioned\0"
  "SPV_NV_shading_rate\0"
  "SPV_NV_stereo_view_rendering\0"
  "SPV_NV_tensor_addressing\0"
  "SPV_NV_viewport_array2\0"
  "SPV_QCOM_cooperative_matrix_conversion\0"
  "SPV_QCOM_image_processing\0"
  "SPV_QCOM_image_processing2\0"
  "SPV_QCOM_image_processing3\0"
  "SPV_QCOM_multiple_wait_queues\0"
  "SPV_QCOM_tile_shading\0"
  "SPV_SAMSUNG_intrinsic\0"
  "SPV_VALIDATOR_ignore_type_decl_unique\0"
  "SPV_VALVE_mixed_float_dot_product\0"
  "ReadOnly\0"
  "Kernel\0"
  "WriteOnly\0"
  "ReadWrite\0"
  "Logical\0"
  "Physical32\0"
  "Addresses\0"
  "Physical64\0"
  "PhysicalStorageBuffer64\0"
  "PhysicalStorageBuffer64EXT\0"
  "PhysicalStorageBufferAddresses\0"
  "Position\0"
  "Shader\0"
  "PointSize\0"
  "ClipDistance\0"
  "CullDistance\0"
  "VertexId\0"
  "InstanceId\0"
  "PrimitiveId\0"
  "Geometry\0"
  "Tessellation\0"
  "RayTracingNV\0"
  "RayTracingKHR\0"
  "MeshShadingNV\0"
  "MeshShadingEXT\0"
  "InvocationId\0"
  "Layer\0"
  "ShaderLayer\0"
  "ShaderViewportIndexLayerEXT\0"
  "ViewportIndex\0"
  "MultiViewport\0"
  "ShaderViewportIndex\0"
  "TessLevelOuter\0"
  "TessLevelInner\0"
  "TessCoord\0"
  "PatchVertices\0"
  "FragCoord\0"
  "PointCoord\0"
  "FrontFacing\0"
  "SampleId\0"
  "SampleRateShading\0"
  "SamplePosition\0"
  "SampleMask\0"
  "FragDepth\0"
  "HelperInvocation\0"
  "NumWorkgroups\0"
  "WorkgroupSize\0"
  "WorkgroupId\0"
  "LocalInvocationId\0"
  "GlobalInvocationId\0"
  "LocalInvocationIndex\0"
  "WorkDim\0"
  "GlobalSize\0"
  "EnqueuedWorkgroupSize\0"
  "GlobalOffset\0"
  "GlobalLinearId\0"
  "SubgroupSize\0"
  "GroupNonUniform\0"
  "SubgroupBallotKHR\0"
  "SubgroupMaxSize\0"
  "NumSubgroups\0"
  "NumEnqueuedSubgroups\0"
  "SubgroupId\0"
  "SubgroupLocalInvocationId\0"
  "VertexIndex\0"
  "InstanceIndex\0"
  "CoreIDARM\0"
  "CoreBuiltinsARM\0"
  "CoreCountARM\0"
  "CoreMaxIDARM\0"
  "WarpIDARM\0"
  "WarpMaxIDARM\0"
  "SubgroupEqMask\0"
  "SubgroupEqMaskKHR\0"
  "GroupNonUniformBallot\0"
  "SubgroupGeMask\0"
  "SubgroupGeMaskKHR\0"
  "SubgroupGtMask\0"
  "SubgroupGtMaskKHR\0"
  "SubgroupLeMask\0"
  "SubgroupLeMaskKHR\0"
  "SubgroupLtMask\0"
  "SubgroupLtMaskKHR\0"
  "BaseVertex\0"
  "DrawParameters\0"
  "BaseInstance\0"
  "DrawIndex\0"
  "PrimitiveShadingRateKHR\0"
  "FragmentShadingRateKHR\0"
  "DeviceIndex\0"
  "DeviceGroup\0"
  "ViewIndex\0"
  "MultiView\0"
  "ShadingRateKHR\0"
  "TileOffsetQCOM\0"
  "TileShadingQCOM\0"
  "TileDimensionQCOM\0"
  "TileApronSizeQCOM\0"
  "BaryCoordNoPerspAMD\0"
  "BaryCoordNoPerspCentroidAMD\0"
  "BaryCoordNoPerspSampleAMD\0"
  "BaryCoordSmoothAMD\0"
  "BaryCoordSmoothCentroidAMD\0"
  "BaryCoordSmoothSampleAMD\0"
  "BaryCoordPullModelAMD\0"
  "FragStencilRefEXT\0"
  "StencilExportEXT\0"
  "RemainingRecursionLevelsAMDX\0"
  "ShaderEnqueueAMDX\0"
  "ShaderIndexAMDX\0"
  "SamplerHeapEXT\0"
  "DescriptorHeapEXT\0"
  "ResourceHeapEXT\0"
  "ViewportMaskNV\0"
  "ShaderViewportMaskNV\0"
  "SecondaryPositionNV\0"
  "ShaderStereoViewNV\0"
  "SecondaryViewportMaskNV\0"
  "PositionPerViewNV\0"
  "PerViewAttributesNV\0"
  "ViewportMaskPerViewNV\0"
  "FullyCoveredEXT\0"
  "FragmentFullyCoveredEXT\0"
  "TaskCountNV\0"
  "PrimitiveCountNV\0"
  "PrimitiveIndicesNV\0"
  "ClipDistancePerViewNV\0"
  "CullDistancePerViewNV\0"
  "LayerPerViewNV\0"
  "MeshViewCountNV\0"
  "MeshViewIndicesNV\0"
  "BaryCoordKHR\0"
  "BaryCoordNV\0"
  "FragmentBarycentricKHR\0"
  "BaryCoordNoPerspKHR\0"
  "BaryCoordNoPerspNV\0"
  "FragSizeEXT\0"
  "FragmentSizeNV\0"
  "FragmentDensityEXT\0"
  "FragInvocationCountEXT\0"
  "InvocationsPerPixelNV\0"
  "PrimitivePointIndicesEXT\0"
  "PrimitiveLineIndicesEXT\0"
  "PrimitiveTriangleIndicesEXT\0"
  "CullPrimitiveEXT\0"
  "LaunchIdKHR\0"
  "LaunchIdNV\0"
  "LaunchSizeKHR\0"
  "LaunchSizeNV\0"
  "WorldRayOriginKHR\0"
  "WorldRayOriginNV\0"
  "WorldRayDirectionKHR\0"
  "WorldRayDirectionNV\0"
  "ObjectRayOriginKHR\0"
  "ObjectRayOriginNV\0"
  "ObjectRayDirectionKHR\0"
  "ObjectRayDirectionNV\0"
  "RayTminKHR\0"
  "RayTminNV\0"
  "RayTmaxKHR\0"
  "RayTmaxNV\0"
  "InstanceCustomIndexKHR\0"
  "InstanceCustomIndexNV\0"
  "ObjectToWorldKHR\0"
  "ObjectToWorldNV\0"
  "WorldToObjectKHR\0"
  "WorldToObjectNV\0"
  "HitTNV\0"
  "HitKindKHR\0"
  "HitKindNV\0"
  "CurrentRayTimeNV\0"
  "RayTracingMotionBlurNV\0"
  "HitTriangleVertexPositionsKHR\0"
  "RayTracingPositionFetchKHR\0"
  "HitMicroTriangleVertexPositionsNV\0"
  "RayTracingDisplacementMicromapNV\0"
  "HitMicroTriangleVertexBarycentricsNV\0"
  "IncomingRayFlagsKHR\0"
  "IncomingRayFlagsNV\0"
  "RayGeometryIndexKHR\0"
  "HitIsSphereNV\0"
  "RayTracingSpheresGeometryNV\0"
  "HitIsLSSNV\0"
  "RayTracingLinearSweptSpheresGeometryNV\0"
  "HitSpherePositionNV\0"
  "WarpsPerSMNV\0"
  "ShaderSMBuiltinsNV\0"
  "SMCountNV\0"
  "WarpIDNV\0"
  "SMIDNV\0"
  "HitLSSPositionsNV\0"
  "HitKindFrontFacingMicroTriangleNV\0"
  "HitKindBackFacingMicroTriangleNV\0"
  "HitSphereRadiusNV\0"
  "HitLSSRadiiNV\0"
  "ClusterIDNV\0"
  "RayTracingClusterAccelerationStructureNV\0"
  "CullMaskKHR\0"
  "RayCullMaskKHR\0"
  "Matrix\0"
  "Linkage\0"
  "Vector16\0"
  "Float16Buffer\0"
  "Float16\0"
  "Float64\0"
  "Int64\0"
  "Int64Atomics\0"
  "ImageBasic\0"
  "ImageReadWrite\0"
  "ImageMipmap\0"
  "Pipes\0"
  "Groups\0"
  "DeviceEnqueue\0"
  "LiteralSampler\0"
  "AtomicStorage\0"
  "Int16\0"
  "TessellationPointSize\0"
  "GeometryPointSize\0"
  "ImageGatherExtended\0"
  "StorageImageMultisample\0"
  "UniformBufferArrayDynamicIndexing\0"
  "SampledImageArrayDynamicIndexing\0"
  "StorageBufferArrayDynamicIndexing\0"
  "StorageImageArrayDynamicIndexing\0"
  "ImageCubeArray\0"
  "SampledCubeArray\0"
  "ImageRect\0"
  "SampledRect\0"
  "GenericPointer\0"
  "Int8\0"
  "InputAttachment\0"
  "SparseResidency\0"
  "MinLod\0"
  "Sampled1D\0"
  "Image1D\0"
  "SampledBuffer\0"
  "ImageBuffer\0"
  "ImageMSArray\0"
  "StorageImageExtendedFormats\0"
  "ImageQuery\0"
  "DerivativeControl\0"
  "InterpolationFunction\0"
  "TransformFeedback\0"
  "GeometryStreams\0"
  "StorageImageReadWithoutFormat\0"
  "StorageImageWriteWithoutFormat\0"
  "SubgroupDispatch\0"
  "NamedBarrier\0"
  "PipeStorage\0"
  "GroupNonUniformVote\0"
  "GroupNonUniformArithmetic\0"
  "GroupNonUniformShuffle\0"
  "GroupNonUniformShuffleRelative\0"
  "GroupNonUniformClustered\0"
  "GroupNonUniformQuad\0"
  "UniformDecoration\0"
  "TileImageColorReadAccessEXT\0"
  "TileImageDepthReadAccessEXT\0"
  "TileImageStencilReadAccessEXT\0"
  "TensorsARM\0"
  "StorageTensorArrayDynamicIndexingARM\0"
  "StorageTensorArrayNonUniformIndexingARM\0"
  "GraphARM\0"
  "CooperativeMatrixLayoutsARM\0"
  "Float8EXT\0"
  "Float8CooperativeMatrixEXT\0"
  "CooperativeMatrixKHR\0"
  "Float6EXT\0"
  "Float4EXT\0"
  "Float8UnsignedE8M0EXT\0"
  "MXInt8EXT\0"
  "BitcastExtractEXT\0"
  "WorkgroupMemoryExplicitLayoutKHR\0"
  "WorkgroupMemoryExplicitLayout8BitAccessKHR\0"
  "WorkgroupMemoryExplicitLayout16BitAccessKHR\0"
  "SubgroupVoteKHR\0"
  "StorageBuffer16BitAccess\0"
  "StorageUniformBufferBlock16\0"
  "UniformAndStorageBuffer16BitAccess\0"
  "StorageUniform16\0"
  "StoragePushConstant16\0"
  "StorageInputOutput16\0"
  "VariablePointersStorageBuffer\0"
  "VariablePointers\0"
  "AtomicStorageOps\0"
  "SampleMaskPostDepthCoverage\0"
  "StorageBuffer8BitAccess\0"
  "UniformAndStorageBuffer8BitAccess\0"
  "StoragePushConstant8\0"
  "DenormPreserve\0"
  "DenormFlushToZero\0"
  "SignedZeroInfNanPreserve\0"
  "RoundingModeRTE\0"
  "RoundingModeRTZ\0"
  "RayQueryProvisionalKHR\0"
  "RayQueryKHR\0"
  "UntypedPointersKHR\0"
  "RayTraversalPrimitiveCullingKHR\0"
  "TextureSampleWeightedQCOM\0"
  "TextureBoxFilterQCOM\0"
  "TextureBlockMatchQCOM\0"
  "CooperativeMatrixConversionQCOM\0"
  "TextureBlockMatch2QCOM\0"
  "MultipleWaitQueuesQCOM\0"
  "ImageGatherLinearQCOM\0"
  "ImageGatherExtendedModesQCOM\0"
  "Float16ImageAMD\0"
  "ImageGatherBiasLodAMD\0"
  "FragmentMaskAMD\0"
  "ImageReadWriteLodAMD\0"
  "Int64ImageEXT\0"
  "ShaderClockKHR\0"
  "QuadControlKHR\0"
  "Int4TypeINTEL\0"
  "Int4CooperativeMatrixINTEL\0"
  "BFloat16TypeKHR\0"
  "BFloat16DotProductKHR\0"
  "BFloat16CooperativeMatrixKHR\0"
  "AbortKHR\0"
  "ConstantDataKHR\0"
  "PoisonFreezeKHR\0"
  "WeakLinkageAMD\0"
  "SampleMaskOverrideCoverageNV\0"
  "GeometryShaderPassthroughNV\0"
  "ShaderViewportIndexLayerNV\0"
  "ImageFootprintNV\0"
  "FragmentBarycentricNV\0"
  "ComputeDerivativeGroupQuadsKHR\0"
  "ComputeDerivativeGroupQuadsNV\0"
  "ShadingRateNV\0"
  "GroupNonUniformPartitionedEXT\0"
  "GroupNonUniformPartitionedNV\0"
  "ShaderNonUniform\0"
  "ShaderNonUniformEXT\0"
  "RuntimeDescriptorArray\0"
  "RuntimeDescriptorArrayEXT\0"
  "InputAttachmentArrayDynamicIndexing\0"
  "InputAttachmentArrayDynamicIndexingEXT\0"
  "UniformTexelBufferArrayDynamicIndexing\0"
  "UniformTexelBufferArrayDynamicIndexingEXT\0"
  "StorageTexelBufferArrayDynamicIndexing\0"
  "StorageTexelBufferArrayDynamicIndexingEXT\0"
  "UniformBufferArrayNonUniformIndexing\0"
  "UniformBufferArrayNonUniformIndexingEXT\0"
  "SampledImageArrayNonUniformIndexing\0"
  "SampledImageArrayNonUniformIndexingEXT\0"
  "StorageBufferArrayNonUniformIndexing\0"
  "StorageBufferArrayNonUniformIndexingEXT\0"
  "StorageImageArrayNonUniformIndexing\0"
  "StorageImageArrayNonUniformIndexingEXT\0"
  "InputAttachmentArrayNonUniformIndexing\0"
  "InputAttachmentArrayNonUniformIndexingEXT\0"
  "UniformTexelBufferArrayNonUniformIndexing\0"
  "UniformTexelBufferArrayNonUniformIndexingEXT\0"
  "StorageTexelBufferArrayNonUniformIndexing\0"
  "StorageTexelBufferArrayNonUniformIndexingEXT\0"
  "VulkanMemoryModel\0"
  "VulkanMemoryModelKHR\0"
  "VulkanMemoryModelDeviceScope\0"
  "VulkanMemoryModelDeviceScopeKHR\0"
  "PhysicalStorageBufferAddressesEXT\0"
  "ComputeDerivativeGroupLinearKHR\0"
  "ComputeDerivativeGroupLinearNV\0"
  "RayTracingProvisionalKHR\0"
  "CooperativeMatrixNV\0"
  "FragmentShaderSampleInterlockEXT\0"
  "FragmentShaderShadingRateInterlockEXT\0"
  "FragmentShaderPixelInterlockEXT\0"
  "DemoteToHelperInvocation\0"
  "DemoteToHelperInvocationEXT\0"
  "DisplacementMicromapNV\0"
  "RayTracingOpacityMicromapKHR\0"
  "RayTracingOpacityMicromapEXT\0"
  "ShaderInvocationReorderNV\0"
  "ShaderInvocationReorderEXT\0"
  "BindlessTextureNV\0"
  "RayQueryPositionFetchKHR\0"
  "CooperativeVectorNV\0"
  "AtomicFloat16VectorNV\0"
  "RawAccessChainsNV\0"
  "PushConstantBanksNV\0"
  "LongVectorEXT\0"
  "Shader64BitIndexingEXT\0"
  "CooperativeMatrixReductionsNV\0"
  "CooperativeMatrixConversionsNV\0"
  "CooperativeMatrixPerElementOperationsNV\0"
  "CooperativeMatrixTensorAddressingNV\0"
  "CooperativeMatrixBlockLoadsNV\0"
  "CooperativeVectorTrainingNV\0"
  "TensorAddressingNV\0"
  "CooperativeMatrixDecodeVectorNV\0"
  "SubgroupShuffleINTEL\0"
  "SubgroupBufferBlockIOINTEL\0"
  "SubgroupImageBlockIOINTEL\0"
  "SubgroupImageMediaBlockIOINTEL\0"
  "RoundToInfinityINTEL\0"
  "FloatingPointModeINTEL\0"
  "IntegerFunctions2INTEL\0"
  "FunctionPointersINTEL\0"
  "IndirectReferencesINTEL\0"
  "AsmINTEL\0"
  "AtomicFloat32MinMaxEXT\0"
  "AtomicFloat64MinMaxEXT\0"
  "AtomicFloat16MinMaxEXT\0"
  "VectorComputeINTEL\0"
  "VectorAnyINTEL\0"
  "ExpectAssumeKHR\0"
  "SubgroupAvcMotionEstimationINTEL\0"
  "SubgroupAvcMotionEstimationIntraINTEL\0"
  "SubgroupAvcMotionEstimationChromaINTEL\0"
  "VariableLengthArrayINTEL\0"
  "FunctionFloatControlINTEL\0"
  "FPGAMemoryAttributesALTERA\0"
  "FPGAMemoryAttributesINTEL\0"
  "FPFastMathModeINTEL\0"
  "ArbitraryPrecisionIntegersALTERA\0"
  "ArbitraryPrecisionIntegersINTEL\0"
  "ArbitraryPrecisionFloatingPointALTERA\0"
  "ArbitraryPrecisionFloatingPointINTEL\0"
  "UnstructuredLoopControlsINTEL\0"
  "FPGALoopControlsALTERA\0"
  "FPGALoopControlsINTEL\0"
  "KernelAttributesINTEL\0"
  "FPGAKernelAttributesINTEL\0"
  "FPGAMemoryAccessesALTERA\0"
  "FPGAMemoryAccessesINTEL\0"
  "FPGAClusterAttributesALTERA\0"
  "FPGAClusterAttributesINTEL\0"
  "LoopFuseALTERA\0"
  "LoopFuseINTEL\0"
  "FPGADSPControlALTERA\0"
  "FPGADSPControlINTEL\0"
  "MemoryAccessAliasingINTEL\0"
  "FPGAInvocationPipeliningAttributesALTERA\0"
  "FPGAInvocationPipeliningAttributesINTEL\0"
  "FPGABufferLocationALTERA\0"
  "FPGABufferLocationINTEL\0"
  "ArbitraryPrecisionFixedPointALTERA\0"
  "ArbitraryPrecisionFixedPointINTEL\0"
  "USMStorageClassesALTERA\0"
  "USMStorageClassesINTEL\0"
  "RuntimeAlignedAttributeALTERA\0"
  "RuntimeAlignedAttributeINTEL\0"
  "IOPipesALTERA\0"
  "IOPipesINTEL\0"
  "BlockingPipesALTERA\0"
  "BlockingPipesINTEL\0"
  "FPGARegALTERA\0"
  "FPGARegINTEL\0"
  "DotProductInputAll\0"
  "DotProductInputAllKHR\0"
  "DotProductInput4x8Bit\0"
  "DotProductInput4x8BitKHR\0"
  "DotProductInput4x8BitPacked\0"
  "DotProductInput4x8BitPackedKHR\0"
  "DotProduct\0"
  "DotProductKHR\0"
  "ReplicatedCompositesEXT\0"
  "BitInstructions\0"
  "GroupNonUniformRotateKHR\0"
  "FloatControls2\0"
  "FMAKHR\0"
  "RayTracingOpacityMicromapExecutionModeKHR\0"
  "AtomicFloat32AddEXT\0"
  "AtomicFloat64AddEXT\0"
  "LongCompositesINTEL\0"
  "OptNoneEXT\0"
  "OptNoneINTEL\0"
  "AtomicFloat16AddEXT\0"
  "DebugInfoModuleINTEL\0"
  "BFloat16ConversionINTEL\0"
  "SplitBarrierEXT\0"
  "SplitBarrierINTEL\0"
  "ArithmeticFenceEXT\0"
  "FPGAClusterAttributesV2ALTERA\0"
  "FPGAClusterAttributesV2INTEL\0"
  "FPGAKernelAttributesv2INTEL\0"
  "TaskSequenceALTERA\0"
  "TaskSequenceINTEL\0"
  "FPMaxErrorINTEL\0"
  "FPGALatencyControlALTERA\0"
  "FPGALatencyControlINTEL\0"
  "FPGAArgumentInterfacesALTERA\0"
  "FPGAArgumentInterfacesINTEL\0"
  "GlobalVariableHostAccessINTEL\0"
  "GlobalVariableFPGADecorationsALTERA\0"
  "GlobalVariableFPGADecorationsINTEL\0"
  "SubgroupBufferPrefetchINTEL\0"
  "Subgroup2DBlockIOINTEL\0"
  "Subgroup2DBlockTransformINTEL\0"
  "Subgroup2DBlockTransposeINTEL\0"
  "SubgroupMatrixMultiplyAccumulateINTEL\0"
  "TernaryBitwiseFunctionINTEL\0"
  "UntypedVariableLengthArrayINTEL\0"
  "SpecConditionalINTEL\0"
  "FunctionVariantsINTEL\0"
  "PredicatedIOINTEL\0"
  "RoundedDivideSqrtINTEL\0"
  "GroupUniformArithmeticKHR\0"
  "TensorFloat32RoundingINTEL\0"
  "MaskedGatherScatterINTEL\0"
  "CacheControlsINTEL\0"
  "RegisterLimitsINTEL\0"
  "BindlessImagesINTEL\0"
  "DotProductFloat16AccFloat32VALVE\0"
  "DotProductFloat16AccFloat16VALVE\0"
  "DotProductBFloat16AccVALVE\0"
  "DotProductFloat8AccFloat32VALVE\0"
  "IntrinsicSAMSUNG\0"
  "Unspecified\0"
  "Address\0"
  "Boolean\0"
  "Float\0"
  "Signed\0"
  "SignedChar\0"
  "Unsigned\0"
  "UnsignedChar\0"
  "Class\0"
  "Structure\0"
  "Union\0"
  "ImportedModule\0"
  "ImportedDeclaration\0"
  "None\0"
  "FlagIsProtected\0"
  "FlagIsPrivate\0"
  "FlagIsPublic\0"
  "FlagIsLocal\0"
  "FlagIsDefinition\0"
  "FlagFwdDecl\0"
  "FlagArtificial\0"
  "FlagExplicit\0"
  "FlagPrototyped\0"
  "FlagObjectPointer\0"
  "FlagStaticMember\0"
  "FlagIndirectVariable\0"
  "FlagLValueReference\0"
  "FlagRValueReference\0"
  "FlagIsOptimized\0"
  "FlagIsEnumClass\0"
  "FlagTypePassByValue\0"
  "FlagTypePassByReference\0"
  "Deref\0"
  "Plus\0"
  "Minus\0"
  "SPV_OPERAND_TYPE_LITERAL_INTEGER\0"
  "PlusUconst\0"
  "BitPiece\0"
  "Swap\0"
  "Xderef\0"
  "StackValue\0"
  "Constu\0"
  "Fragment\0"
  "ConstType\0"
  "VolatileType\0"
  "RestrictType\0"
  "AtomicType\0"
  "Float16NV\0"
  "Float32NV\0"
  "Float64NV\0"
  "SignedInt8NV\0"
  "SignedInt16NV\0"
  "SignedInt32NV\0"
  "SignedInt64NV\0"
  "UnsignedInt8NV\0"
  "UnsignedInt16NV\0"
  "UnsignedInt32NV\0"
  "UnsignedInt64NV\0"
  "SignedInt8PackedNV\0"
  "UnsignedInt8PackedNV\0"
  "FloatE4M3NV\0"
  "FloatE5M2NV\0"
  "RowMajorKHR\0"
  "ColumnMajorKHR\0"
  "RowBlockedInterleavedARM\0"
  "ColumnBlockedInterleavedARM\0"
  "NoneKHR\0"
  "MatrixASignedComponentsKHR\0"
  "MatrixBSignedComponentsKHR\0"
  "MatrixCSignedComponentsKHR\0"
  "MatrixResultSignedComponentsKHR\0"
  "SaturatingAccumulationKHR\0"
  "Row\0"
  "Column\0"
  "2x2\0"
  "MatrixAKHR\0"
  "MatrixBKHR\0"
  "MatrixAccumulatorKHR\0"
  "RowMajorNV\0"
  "ColumnMajorNV\0"
  "InferencingOptimalNV\0"
  "TrainingOptimalNV\0"
  "RelaxedPrecision\0"
  "SpecId\0"
  "Block\0"
  "BufferBlock\0"
  "RowMajor\0"
  "ColMajor\0"
  "ArrayStride\0"
  "MatrixStride\0"
  "GLSLShared\0"
  "GLSLPacked\0"
  "CPacked\0"
  "SPV_OPERAND_TYPE_BUILT_IN\0"
  "BuiltIn\0"
  "NoPerspective\0"
  "Flat\0"
  "Patch\0"
  "Centroid\0"
  "Sample\0"
  "Invariant\0"
  "Restrict\0"
  "Aliased\0"
  "Volatile\0"
  "Constant\0"
  "Coherent\0"
  "NonWritable\0"
  "NonReadable\0"
  "Uniform\0"
  "SPV_OPERAND_TYPE_SCOPE_ID\0"
  "UniformId\0"
  "SaturatedConversion\0"
  "Stream\0"
  "Location\0"
  "Component\0"
  "Index\0"
  "Binding\0"
  "DescriptorSet\0"
  "Offset\0"
  "XfbBuffer\0"
  "XfbStride\0"
  "SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE\0"
  "FuncParamAttr\0"
  "SPV_OPERAND_TYPE_FP_ROUNDING_MODE\0"
  "FPRoundingMode\0"
  "SPV_OPERAND_TYPE_FP_FAST_MATH_MODE\0"
  "FPFastMathMode\0"
  "SPV_OPERAND_TYPE_LITERAL_STRING\0"
  "SPV_OPERAND_TYPE_LINKAGE_TYPE\0"
  "LinkageAttributes\0"
  "NoContraction\0"
  "InputAttachmentIndex\0"
  "Alignment\0"
  "MaxByteOffset\0"
  "SPV_OPERAND_TYPE_ID\0"
  "AlignmentId\0"
  "MaxByteOffsetId\0"
  "SaturatedToLargestFloat8NormalConversionEXT\0"
  "NoSignedWrap\0"
  "NoUnsignedWrap\0"
  "WeightTextureQCOM\0"
  "BlockMatchTextureQCOM\0"
  "BlockMatchSamplerQCOM\0"
  "ExplicitInterpAMD\0"
  "NodeSharesPayloadLimitsWithAMDX\0"
  "NodeMaxPayloadsAMDX\0"
  "TrackFinishWritingAMDX\0"
  "PayloadNodeNameAMDX\0"
  "PayloadNodeBaseIndexAMDX\0"
  "PayloadNodeSparseArrayAMDX\0"
  "PayloadNodeArraySizeAMDX\0"
  "PayloadDispatchIndirectAMDX\0"
  "ArrayStrideIdEXT\0"
  "OffsetIdEXT\0"
  "UTFEncodedKHR\0"
  "OverrideCoverageNV\0"
  "PassthroughNV\0"
  "ViewportRelativeNV\0"
  "SecondaryViewportRelativeNV\0"
  "PerPrimitiveEXT\0"
  "PerPrimitiveNV\0"
  "PerViewNV\0"
  "PerTaskNV\0"
  "PerVertexKHR\0"
  "PerVertexNV\0"
  "NonUniform\0"
  "NonUniformEXT\0"
  "RestrictPointer\0"
  "RestrictPointerEXT\0"
  "AliasedPointer\0"
  "AliasedPointerEXT\0"
  "MemberOffsetNV\0"
  "HitObjectShaderRecordBufferNV\0"
  "HitObjectShaderRecordBufferEXT\0"
  "BankNV\0"
  "BindlessSamplerNV\0"
  "BindlessImageNV\0"
  "BoundSamplerNV\0"
  "BoundImageNV\0"
  "SIMTCallINTEL\0"
  "ReferencedIndirectlyINTEL\0"
  "ClobberINTEL\0"
  "SideEffectsINTEL\0"
  "VectorComputeVariableINTEL\0"
  "FuncParamIOKindINTEL\0"
  "VectorComputeFunctionINTEL\0"
  "StackCallINTEL\0"
  "GlobalVariableOffsetINTEL\0"
  "CounterBuffer\0"
  "HlslCounterBufferGOOGLE\0"
  "UserSemantic\0"
  "HlslSemanticGOOGLE\0"
  "UserTypeGOOGLE\0"
  "FunctionRoundingModeINTEL\0"
  "SPV_OPERAND_TYPE_FPDENORM_MODE\0"
  "FunctionDenormModeINTEL\0"
  "RegisterALTERA\0"
  "RegisterINTEL\0"
  "MemoryALTERA\0"
  "MemoryINTEL\0"
  "NumbanksALTERA\0"
  "NumbanksINTEL\0"
  "BankwidthALTERA\0"
  "BankwidthINTEL\0"
  "MaxPrivateCopiesALTERA\0"
  "MaxPrivateCopiesINTEL\0"
  "SinglepumpALTERA\0"
  "SinglepumpINTEL\0"
  "DoublepumpALTERA\0"
  "DoublepumpINTEL\0"
  "MaxReplicatesALTERA\0"
  "MaxReplicatesINTEL\0"
  "SimpleDualPortALTERA\0"
  "SimpleDualPortINTEL\0"
  "MergeALTERA\0"
  "MergeINTEL\0"
  "SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER\0"
  "BankBitsALTERA\0"
  "BankBitsINTEL\0"
  "ForcePow2DepthALTERA\0"
  "ForcePow2DepthINTEL\0"
  "StridesizeALTERA\0"
  "StridesizeINTEL\0"
  "WordsizeALTERA\0"
  "WordsizeINTEL\0"
  "TrueDualPortALTERA\0"
  "TrueDualPortINTEL\0"
  "BurstCoalesceALTERA\0"
  "BurstCoalesceINTEL\0"
  "CacheSizeALTERA\0"
  "CacheSizeINTEL\0"
  "DontStaticallyCoalesceALTERA\0"
  "DontStaticallyCoalesceINTEL\0"
  "PrefetchALTERA\0"
  "PrefetchINTEL\0"
  "StallEnableALTERA\0"
  "StallEnableINTEL\0"
  "FuseLoopsInFunctionALTERA\0"
  "FuseLoopsInFunctionINTEL\0"
  "MathOpDSPModeALTERA\0"
  "MathOpDSPModeINTEL\0"
  "AliasScopeINTEL\0"
  "NoAliasINTEL\0"
  "InitiationIntervalALTERA\0"
  "InitiationIntervalINTEL\0"
  "MaxConcurrencyALTERA\0"
  "MaxConcurrencyINTEL\0"
  "PipelineEnableALTERA\0"
  "PipelineEnableINTEL\0"
  "BufferLocationALTERA\0"
  "BufferLocationINTEL\0"
  "IOPipeStorageALTERA\0"
  "IOPipeStorageINTEL\0"
  "SPV_OPERAND_TYPE_FPOPERATION_MODE\0"
  "FunctionFloatingPointModeINTEL\0"
  "SingleElementVectorINTEL\0"
  "VectorComputeCallableFunctionINTEL\0"
  "MediaBlockIOINTEL\0"
  "StallFreeALTERA\0"
  "StallFreeINTEL\0"
  "SPV_OPERAND_TYPE_LITERAL_FLOAT\0"
  "FPMaxErrorDecorationINTEL\0"
  "LatencyControlLabelALTERA\0"
  "LatencyControlLabelINTEL\0"
  "LatencyControlConstraintALTERA\0"
  "LatencyControlConstraintINTEL\0"
  "ConduitKernelArgumentALTERA\0"
  "ConduitKernelArgumentINTEL\0"
  "RegisterMapKernelArgumentALTERA\0"
  "RegisterMapKernelArgumentINTEL\0"
  "MMHostInterfaceAddressWidthALTERA\0"
  "MMHostInterfaceAddressWidthINTEL\0"
  "MMHostInterfaceDataWidthALTERA\0"
  "MMHostInterfaceDataWidthINTEL\0"
  "MMHostInterfaceLatencyALTERA\0"
  "MMHostInterfaceLatencyINTEL\0"
  "SPV_OPERAND_TYPE_ACCESS_QUALIFIER\0"
  "MMHostInterfaceReadWriteModeALTERA\0"
  "MMHostInterfaceReadWriteModeINTEL\0"
  "MMHostInterfaceMaxBurstALTERA\0"
  "MMHostInterfaceMaxBurstINTEL\0"
  "MMHostInterfaceWaitRequestALTERA\0"
  "MMHostInterfaceWaitRequestINTEL\0"
  "StableKernelArgumentALTERA\0"
  "StableKernelArgumentINTEL\0"
  "SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER\0"
  "HostAccessINTEL\0"
  "SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER\0"
  "InitModeALTERA\0"
  "InitModeINTEL\0"
  "ImplementInRegisterMapALTERA\0"
  "ImplementInRegisterMapINTEL\0"
  "ConditionalINTEL\0"
  "SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL\0"
  "CacheControlLoadINTEL\0"
  "SPV_OPERAND_TYPE_STORE_CACHE_CONTROL\0"
  "CacheControlStoreINTEL\0"
  "1D\0"
  "2D\0"
  "3D\0"
  "Cube\0"
  "Rect\0"
  "Buffer\0"
  "SubpassData\0"
  "TileImageDataEXT\0"
  "Invocations\0"
  "SpacingEqual\0"
  "SpacingFractionalEven\0"
  "SpacingFractionalOdd\0"
  "VertexOrderCw\0"
  "VertexOrderCcw\0"
  "PixelCenterInteger\0"
  "OriginUpperLeft\0"
  "OriginLowerLeft\0"
  "EarlyFragmentTests\0"
  "PointMode\0"
  "Xfb\0"
  "DepthReplacing\0"
  "DepthGreater\0"
  "DepthLess\0"
  "DepthUnchanged\0"
  "LocalSize\0"
  "LocalSizeHint\0"
  "InputPoints\0"
  "InputLines\0"
  "InputLinesAdjacency\0"
  "Triangles\0"
  "InputTrianglesAdjacency\0"
  "Quads\0"
  "Isolines\0"
  "OutputVertices\0"
  "OutputPoints\0"
  "OutputLineStrip\0"
  "OutputTriangleStrip\0"
  "VecTypeHint\0"
  "ContractionOff\0"
  "Initializer\0"
  "Finalizer\0"
  "SubgroupsPerWorkgroup\0"
  "SubgroupsPerWorkgroupId\0"
  "LocalSizeId\0"
  "LocalSizeHintId\0"
  "NonCoherentColorAttachmentReadEXT\0"
  "NonCoherentDepthAttachmentReadEXT\0"
  "NonCoherentStencilAttachmentReadEXT\0"
  "SubgroupUniformControlFlowKHR\0"
  "PostDepthCoverage\0"
  "NonCoherentTileAttachmentReadQCOM\0"
  "TileShadingRateQCOM\0"
  "EarlyAndLateFragmentTestsAMD\0"
  "StencilRefReplacingEXT\0"
  "CoalescingAMDX\0"
  "IsApiEntryAMDX\0"
  "MaxNodeRecursionAMDX\0"
  "StaticNumWorkgroupsAMDX\0"
  "MaxNumWorkgroupsAMDX\0"
  "StencilRefUnchangedFrontAMD\0"
  "StencilRefGreaterFrontAMD\0"
  "StencilRefLessFrontAMD\0"
  "StencilRefUnchangedBackAMD\0"
  "StencilRefGreaterBackAMD\0"
  "StencilRefLessBackAMD\0"
  "QuadDerivativesKHR\0"
  "RequireFullQuadsKHR\0"
  "SharesInputWithAMDX\0"
  "ArithmeticPoisonKHR\0"
  "OutputLinesEXT\0"
  "OutputLinesNV\0"
  "OutputPrimitivesEXT\0"
  "OutputPrimitivesNV\0"
  "DerivativeGroupQuadsKHR\0"
  "DerivativeGroupQuadsNV\0"
  "DerivativeGroupLinearKHR\0"
  "DerivativeGroupLinearNV\0"
  "OutputTrianglesEXT\0"
  "OutputTrianglesNV\0"
  "PixelInterlockOrderedEXT\0"
  "PixelInterlockUnorderedEXT\0"
  "SampleInterlockOrderedEXT\0"
  "SampleInterlockUnorderedEXT\0"
  "ShadingRateInterlockOrderedEXT\0"
  "ShadingRateInterlockUnorderedEXT\0"
  "SharedLocalMemorySizeINTEL\0"
  "RoundingModeRTPINTEL\0"
  "RoundingModeRTNINTEL\0"
  "FloatingPointModeALTINTEL\0"
  "FloatingPointModeIEEEINTEL\0"
  "MaxWorkgroupSizeINTEL\0"
  "MaxWorkDimINTEL\0"
  "NoGlobalOffsetINTEL\0"
  "NumSIMDWorkitemsINTEL\0"
  "SchedulerTargetFmaxMhzINTEL\0"
  "MaximallyReconvergesKHR\0"
  "FPFastMathDefault\0"
  "OpacityMicromapIdKHR\0"
  "StreamingInterfaceINTEL\0"
  "RegisterMapInterfaceINTEL\0"
  "NamedBarrierCountINTEL\0"
  "MaximumRegistersINTEL\0"
  "MaximumRegistersIdINTEL\0"
  "SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS\0"
  "NamedMaximumRegistersINTEL\0"
  "Vertex\0"
  "TessellationControl\0"
  "TessellationEvaluation\0"
  "GLCompute\0"
  "TaskNV\0"
  "MeshNV\0"
  "RayGenerationKHR\0"
  "RayGenerationNV\0"
  "IntersectionKHR\0"
  "IntersectionNV\0"
  "AnyHitKHR\0"
  "AnyHitNV\0"
  "ClosestHitKHR\0"
  "ClosestHitNV\0"
  "MissKHR\0"
  "MissNV\0"
  "CallableKHR\0"
  "CallableNV\0"
  "TaskEXT\0"
  "MeshEXT\0"
  "Preserve\0"
  "FlushToZero\0"
  "BFloat16KHR\0"
  "Float8E4M3EXT\0"
  "Float8E5M2EXT\0"
  "Float6E2M3EXT\0"
  "Float6E3M2EXT\0"
  "Float4E2M1EXT\0"
  "IEEE\0"
  "ALT\0"
  "NotNaN\0"
  "NotInf\0"
  "NSZ\0"
  "AllowRecip\0"
  "Fast\0"
  "AllowContract\0"
  "AllowContractFastINTEL\0"
  "AllowReassoc\0"
  "AllowReassocINTEL\0"
  "AllowTransform\0"
  "RTE\0"
  "RTZ\0"
  "RTP\0"
  "RTN\0"
  "Vertical2Pixels\0"
  "Vertical4Pixels\0"
  "Horizontal2Pixels\0"
  "Horizontal4Pixels\0"
  "Inline\0"
  "DontInline\0"
  "Pure\0"
  "Const\0"
  "Zext\0"
  "Sext\0"
  "ByVal\0"
  "Sret\0"
  "NoAlias\0"
  "NoCapture\0"
  "NoWrite\0"
  "NoReadWrite\0"
  "RuntimeAlignedALTERA\0"
  "RuntimeAlignedINTEL\0"
  "Gather4x1QCOM\0"
  "GatherDQCOM\0"
  "GatherH2QCOM\0"
  "GatherV2QCOM\0"
  "Reduce\0"
  "InclusiveScan\0"
  "ExclusiveScan\0"
  "ClusteredReduce\0"
  "PartitionedReduceEXT\0"
  "PartitionedReduceNV\0"
  "PartitionedInclusiveScanEXT\0"
  "PartitionedInclusiveScanNV\0"
  "PartitionedExclusiveScanEXT\0"
  "PartitionedExclusiveScanNV\0"
  "NoneINTEL\0"
  "ReadINTEL\0"
  "WriteINTEL\0"
  "ReadWriteINTEL\0"
  "Bias\0"
  "Lod\0"
  "Grad\0"
  "ConstOffset\0"
  "ConstOffsets\0"
  "MakeTexelAvailable\0"
  "MakeTexelAvailableKHR\0"
  "MakeTexelVisible\0"
  "MakeTexelVisibleKHR\0"
  "NonPrivateTexel\0"
  "NonPrivateTexelKHR\0"
  "VolatileTexel\0"
  "VolatileTexelKHR\0"
  "SignExtend\0"
  "ZeroExtend\0"
  "Nontemporal\0"
  "Offsets\0"
  "SnormInt8\0"
  "SnormInt16\0"
  "UnormInt8\0"
  "UnormInt16\0"
  "UnormShort565\0"
  "UnormShort555\0"
  "UnormInt101010\0"
  "SignedInt8\0"
  "SignedInt16\0"
  "SignedInt32\0"
  "UnsignedInt8\0"
  "UnsignedInt16\0"
  "UnsignedInt32\0"
  "HalfFloat\0"
  "UnormInt24\0"
  "UnormInt101010_2\0"
  "UnormInt10X6EXT\0"
  "UnsignedIntRaw10EXT\0"
  "UnsignedIntRaw12EXT\0"
  "UnormInt2_101010EXT\0"
  "UnsignedInt10X6EXT\0"
  "UnsignedInt12X4EXT\0"
  "UnsignedInt14X2EXT\0"
  "UnormInt12X4EXT\0"
  "UnormInt14X2EXT\0"
  "R\0"
  "A\0"
  "RG\0"
  "RA\0"
  "RGB\0"
  "RGBA\0"
  "BGRA\0"
  "ARGB\0"
  "Intensity\0"
  "Luminance\0"
  "Rx\0"
  "RGx\0"
  "RGBx\0"
  "Depth\0"
  "DepthStencil\0"
  "sRGB\0"
  "sRGBx\0"
  "sRGBA\0"
  "sBGRA\0"
  "ABGR\0"
  "InitOnDeviceReprogramALTERA\0"
  "InitOnDeviceReprogramINTEL\0"
  "InitOnDeviceResetALTERA\0"
  "InitOnDeviceResetINTEL\0"
  "NoWait\0"
  "WaitKernel\0"
  "WaitWorkGroup\0"
  "CmdExecTime\0"
  "MayUsePrintf\0"
  "Export\0"
  "Import\0"
  "LinkOnceODR\0"
  "WeakAMD\0"
  "UncachedINTEL\0"
  "CachedINTEL\0"
  "StreamingINTEL\0"
  "InvalidateAfterReadINTEL\0"
  "ConstCachedINTEL\0"
  "Unroll\0"
  "DontUnroll\0"
  "DependencyInfinite\0"
  "DependencyLength\0"
  "MinIterations\0"
  "MaxIterations\0"
  "IterationMultiple\0"
  "PeelCount\0"
  "PartialCount\0"
  "DependencyArrayALTERA\0"
  "DependencyArrayINTEL\0"
  "LoopCoalesceALTERA\0"
  "LoopCoalesceINTEL\0"
  "MaxInterleavingALTERA\0"
  "MaxInterleavingINTEL\0"
  "SpeculatedIterationsALTERA\0"
  "SpeculatedIterationsINTEL\0"
  "NoFusionALTERA\0"
  "NoFusionINTEL\0"
  "LoopCountALTERA\0"
  "LoopCountINTEL\0"
  "MaxReinvocationDelayALTERA\0"
  "MaxReinvocationDelayINTEL\0"
  "MatrixASignedComponentsINTEL\0"
  "MatrixBSignedComponentsINTEL\0"
  "MatrixCBFloat16INTEL\0"
  "MatrixResultBFloat16INTEL\0"
  "MatrixAPackedInt8INTEL\0"
  "MatrixBPackedInt8INTEL\0"
  "MatrixAPackedInt4INTEL\0"
  "MatrixBPackedInt4INTEL\0"
  "MatrixATF32INTEL\0"
  "MatrixBTF32INTEL\0"
  "MatrixAPackedFloat16INTEL\0"
  "MatrixBPackedFloat16INTEL\0"
  "MatrixAPackedBFloat16INTEL\0"
  "MatrixBPackedBFloat16INTEL\0"
  "Aligned\0"
  "MakePointerAvailable\0"
  "MakePointerAvailableKHR\0"
  "MakePointerVisible\0"
  "MakePointerVisibleKHR\0"
  "NonPrivatePointer\0"
  "NonPrivatePointerKHR\0"
  "AliasScopeINTELMask\0"
  "NoAliasINTELMask\0"
  "Simple\0"
  "GLSL450\0"
  "OpenCL\0"
  "Vulkan\0"
  "VulkanKHR\0"
  "Relaxed\0"
  "Acquire\0"
  "Release\0"
  "AcquireRelease\0"
  "SequentiallyConsistent\0"
  "UniformMemory\0"
  "SubgroupMemory\0"
  "WorkgroupMemory\0"
  "CrossWorkgroupMemory\0"
  "AtomicCounterMemory\0"
  "ImageMemory\0"
  "OutputMemory\0"
  "OutputMemoryKHR\0"
  "MakeAvailable\0"
  "MakeAvailableKHR\0"
  "MakeVisible\0"
  "MakeVisibleKHR\0"
  "AutoINTEL\0"
  "WRAP\0"
  "SAT\0"
  "SAT_ZERO\0"
  "SAT_SYM\0"
  "PackedVectorFormat4x8Bit\0"
  "PackedVectorFormat4x8BitKHR\0"
  "TRN\0"
  "TRN_ZERO\0"
  "RND\0"
  "RND_ZERO\0"
  "RND_INF\0"
  "RND_MIN_INF\0"
  "RND_CONV\0"
  "RND_CONV_ODD\0"
  "RobustnessPerComponentNV\0"
  "RobustnessPerElementNV\0"
  "OpaqueKHR\0"
  "NoOpaqueKHR\0"
  "TerminateOnFirstHitKHR\0"
  "SkipClosestHitShaderKHR\0"
  "CullBackFacingTrianglesKHR\0"
  "CullFrontFacingTrianglesKHR\0"
  "CullOpaqueKHR\0"
  "CullNoOpaqueKHR\0"
  "SkipTrianglesKHR\0"
  "SkipBuiltinPrimitivesNV\0"
  "SkipAABBsKHR\0"
  "ForceOpacityMicromap2StateKHR\0"
  "ForceOpacityMicromap2StateEXT\0"
  "RayQueryCandidateIntersectionTriangleKHR\0"
  "RayQueryCandidateIntersectionAABBKHR\0"
  "RayQueryCommittedIntersectionNoneKHR\0"
  "RayQueryCommittedIntersectionTriangleKHR\0"
  "RayQueryCommittedIntersectionGeneratedKHR\0"
  "RayQueryCandidateIntersectionKHR\0"
  "RayQueryCommittedIntersectionKHR\0"
  "ClampToEdge\0"
  "Clamp\0"
  "Repeat\0"
  "RepeatMirrored\0"
  "Nearest\0"
  "Linear\0"
  "Unknown\0"
  "Rgba32f\0"
  "Rgba16f\0"
  "R32f\0"
  "Rgba8\0"
  "Rgba8Snorm\0"
  "Rg32f\0"
  "Rg16f\0"
  "R11fG11fB10f\0"
  "R16f\0"
  "Rgba16\0"
  "Rgb10A2\0"
  "Rg16\0"
  "Rg8\0"
  "R16\0"
  "R8\0"
  "Rgba16Snorm\0"
  "Rg16Snorm\0"
  "Rg8Snorm\0"
  "R16Snorm\0"
  "R8Snorm\0"
  "Rgba32i\0"
  "Rgba16i\0"
  "Rgba8i\0"
  "R32i\0"
  "Rg32i\0"
  "Rg16i\0"
  "Rg8i\0"
  "R16i\0"
  "R8i\0"
  "Rgba32ui\0"
  "Rgba16ui\0"
  "Rgba8ui\0"
  "R32ui\0"
  "Rgb10a2ui\0"
  "Rg32ui\0"
  "Rg16ui\0"
  "Rg8ui\0"
  "R16ui\0"
  "R8ui\0"
  "R64ui\0"
  "R64i\0"
  "CrossDevice\0"
  "Device\0"
  "Workgroup\0"
  "Subgroup\0"
  "Invocation\0"
  "QueueFamily\0"
  "QueueFamilyKHR\0"
  "ShaderCallKHR\0"
  "Flatten\0"
  "DontFlatten\0"
  "IdentifierPossibleDuplicates\0"
  "FlagUnknownPhysicalLayout\0"
  "ESSL\0"
  "GLSL\0"
  "OpenCL_C\0"
  "OpenCL_CPP\0"
  "HLSL\0"
  "CPP_for_OpenCL\0"
  "SYCL\0"
  "HERO_C\0"
  "NZSL\0"
  "WGSL\0"
  "Slang\0"
  "Zig\0"
  "Rust\0"
  "Pred\0"
  "ApilaJai\0"
  "UniformConstant\0"
  "Input\0"
  "Output\0"
  "CrossWorkgroup\0"
  "Private\0"
  "Function\0"
  "Generic\0"
  "PushConstant\0"
  "AtomicCounter\0"
  "Image\0"
  "StorageBuffer\0"
  "TileImageEXT\0"
  "TileAttachmentQCOM\0"
  "NodePayloadAMDX\0"
  "CallableDataKHR\0"
  "CallableDataNV\0"
  "IncomingCallableDataKHR\0"
  "IncomingCallableDataNV\0"
  "RayPayloadKHR\0"
  "RayPayloadNV\0"
  "HitAttributeKHR\0"
  "HitAttributeNV\0"
  "IncomingRayPayloadKHR\0"
  "IncomingRayPayloadNV\0"
  "ShaderRecordBufferKHR\0"
  "ShaderRecordBufferNV\0"
  "PhysicalStorageBuffer\0"
  "PhysicalStorageBufferEXT\0"
  "HitObjectAttributeNV\0"
  "TaskPayloadWorkgroupEXT\0"
  "HitObjectAttributeEXT\0"
  "CodeSectionINTEL\0"
  "DeviceOnlyALTERA\0"
  "DeviceOnlyINTEL\0"
  "HostOnlyALTERA\0"
  "HostOnlyINTEL\0"
  "WriteThroughINTEL\0"
  "WriteBackINTEL\0"
  "TensorView\0"
  "DecodeFunc\0"
  "DecodeVectorFunc\0"
  "Undefined\0"
  "NoneARM\0"
  "NontemporalARM\0"
  "OutOfBoundsValueARM\0"
  "MakeElementAvailableARM\0"
  "MakeElementVisibleARM\0"
  "NonPrivateElementARM\0"
  "Nop\0"
  "SPV_OPERAND_TYPE_TYPE_ID\0"
  "SPV_OPERAND_TYPE_RESULT_ID\0"
  "Undef\0"
  "SourceContinued\0"
  "SPV_OPERAND_TYPE_SOURCE_LANGUAGE\0"
  "SPV_OPERAND_TYPE_OPTIONAL_ID\0"
  "SPV_OPERAND_TYPE_OPTIONAL_LITERAL_STRING\0"
  "Source\0"
  "SourceExtension\0"
  "Name\0"
  "MemberName\0"
  "String\0"
  "Line\0"
  "Extension\0"
  "ExtInstImport\0"
  "SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER\0"
  "ExtInst\0"
  "SPV_OPERAND_TYPE_ADDRESSING_MODEL\0"
  "SPV_OPERAND_TYPE_MEMORY_MODEL\0"
  "MemoryModel\0"
  "SPV_OPERAND_TYPE_EXECUTION_MODEL\0"
  "SPV_OPERAND_TYPE_VARIABLE_ID\0"
  "EntryPoint\0"
  "SPV_OPERAND_TYPE_EXECUTION_MODE\0"
  "ExecutionMode\0"
  "SPV_OPERAND_TYPE_CAPABILITY\0"
  "Capability\0"
  "TypeVoid\0"
  "TypeBool\0"
  "TypeInt\0"
  "SPV_OPERAND_TYPE_OPTIONAL_FPENCODING\0"
  "TypeFloat\0"
  "TypeVector\0"
  "TypeMatrix\0"
  "SPV_OPERAND_TYPE_DIMENSIONALITY\0"
  "SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT\0"
  "SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER\0"
  "TypeImage\0"
  "TypeSampler\0"
  "TypeSampledImage\0"
  "TypeArray\0"
  "TypeRuntimeArray\0"
  "TypeStruct\0"
  "TypeOpaque\0"
  "SPV_OPERAND_TYPE_STORAGE_CLASS\0"
  "TypePointer\0"
  "TypeFunction\0"
  "TypeEvent\0"
  "TypeDeviceEvent\0"
  "TypeReserveId\0"
  "TypeQueue\0"
  "TypePipe\0"
  "TypeForwardPointer\0"
  "ConstantTrue\0"
  "ConstantFalse\0"
  "SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER\0"
  "ConstantComposite\0"
  "SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE\0"
  "SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE\0"
  "ConstantSampler\0"
  "ConstantNull\0"
  "SpecConstantTrue\0"
  "SpecConstantFalse\0"
  "SpecConstant\0"
  "SpecConstantComposite\0"
  "SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER\0"
  "SpecConstantOp\0"
  "SPV_OPERAND_TYPE_FUNCTION_CONTROL\0"
  "FunctionParameter\0"
  "FunctionEnd\0"
  "FunctionCall\0"
  "Variable\0"
  "ImageTexelPointer\0"
  "SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS\0"
  "Load\0"
  "Store\0"
  "CopyMemory\0"
  "CopyMemorySized\0"
  "AccessChain\0"
  "InBoundsAccessChain\0"
  "PtrAccessChain\0"
  "ArrayLength\0"
  "GenericPtrMemSemantics\0"
  "InBoundsPtrAccessChain\0"
  "SPV_OPERAND_TYPE_DECORATION\0"
  "Decorate\0"
  "MemberDecorate\0"
  "DecorationGroup\0"
  "GroupDecorate\0"
  "SPV_OPERAND_TYPE_VARIABLE_ID_LITERAL_INTEGER\0"
  "GroupMemberDecorate\0"
  "VectorExtractDynamic\0"
  "VectorInsertDynamic\0"
  "VectorShuffle\0"
  "CompositeConstruct\0"
  "CompositeExtract\0"
  "CompositeInsert\0"
  "CopyObject\0"
  "Transpose\0"
  "SampledImage\0"
  "SPV_OPERAND_TYPE_OPTIONAL_IMAGE\0"
  "ImageSampleImplicitLod\0"
  "SPV_OPERAND_TYPE_IMAGE\0"
  "ImageSampleExplicitLod\0"
  "ImageSampleDrefImplicitLod\0"
  "ImageSampleDrefExplicitLod\0"
  "ImageSampleProjImplicitLod\0"
  "ImageSampleProjExplicitLod\0"
  "ImageSampleProjDrefImplicitLod\0"
  "ImageSampleProjDrefExplicitLod\0"
  "ImageFetch\0"
  "ImageGather\0"
  "ImageDrefGather\0"
  "ImageRead\0"
  "ImageWrite\0"
  "ImageQueryFormat\0"
  "ImageQueryOrder\0"
  "ImageQuerySizeLod\0"
  "ImageQuerySize\0"
  "ImageQueryLod\0"
  "ImageQueryLevels\0"
  "ImageQuerySamples\0"
  "ConvertFToU\0"
  "ConvertFToS\0"
  "ConvertSToF\0"
  "ConvertUToF\0"
  "UConvert\0"
  "SConvert\0"
  "FConvert\0"
  "QuantizeToF16\0"
  "ConvertPtrToU\0"
  "SatConvertSToU\0"
  "SatConvertUToS\0"
  "ConvertUToPtr\0"
  "PtrCastToGeneric\0"
  "GenericCastToPtr\0"
  "GenericCastToPtrExplicit\0"
  "Bitcast\0"
  "SNegate\0"
  "FNegate\0"
  "IAdd\0"
  "FAdd\0"
  "ISub\0"
  "FSub\0"
  "IMul\0"
  "FMul\0"
  "UDiv\0"
  "SDiv\0"
  "FDiv\0"
  "UMod\0"
  "SRem\0"
  "SMod\0"
  "FRem\0"
  "FMod\0"
  "VectorTimesScalar\0"
  "MatrixTimesScalar\0"
  "VectorTimesMatrix\0"
  "MatrixTimesVector\0"
  "MatrixTimesMatrix\0"
  "OuterProduct\0"
  "Dot\0"
  "IAddCarry\0"
  "ISubBorrow\0"
  "UMulExtended\0"
  "SMulExtended\0"
  "Any\0"
  "All\0"
  "IsNan\0"
  "IsInf\0"
  "IsFinite\0"
  "IsNormal\0"
  "SignBitSet\0"
  "LessOrGreater\0"
  "Ordered\0"
  "Unordered\0"
  "LogicalEqual\0"
  "LogicalNotEqual\0"
  "LogicalOr\0"
  "LogicalAnd\0"
  "LogicalNot\0"
  "Select\0"
  "IEqual\0"
  "INotEqual\0"
  "UGreaterThan\0"
  "SGreaterThan\0"
  "UGreaterThanEqual\0"
  "SGreaterThanEqual\0"
  "ULessThan\0"
  "SLessThan\0"
  "ULessThanEqual\0"
  "SLessThanEqual\0"
  "FOrdEqual\0"
  "FUnordEqual\0"
  "FOrdNotEqual\0"
  "FUnordNotEqual\0"
  "FOrdLessThan\0"
  "FUnordLessThan\0"
  "FOrdGreaterThan\0"
  "FUnordGreaterThan\0"
  "FOrdLessThanEqual\0"
  "FUnordLessThanEqual\0"
  "FOrdGreaterThanEqual\0"
  "FUnordGreaterThanEqual\0"
  "ShiftRightLogical\0"
  "ShiftRightArithmetic\0"
  "ShiftLeftLogical\0"
  "BitwiseOr\0"
  "BitwiseXor\0"
  "BitwiseAnd\0"
  "Not\0"
  "BitFieldInsert\0"
  "BitFieldSExtract\0"
  "BitFieldUExtract\0"
  "BitReverse\0"
  "BitCount\0"
  "DPdx\0"
  "DPdy\0"
  "Fwidth\0"
  "DPdxFine\0"
  "DPdyFine\0"
  "FwidthFine\0"
  "DPdxCoarse\0"
  "DPdyCoarse\0"
  "FwidthCoarse\0"
  "EmitVertex\0"
  "EndPrimitive\0"
  "EmitStreamVertex\0"
  "EndStreamPrimitive\0"
  "SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID\0"
  "ControlBarrier\0"
  "MemoryBarrier\0"
  "AtomicLoad\0"
  "AtomicStore\0"
  "AtomicExchange\0"
  "AtomicCompareExchange\0"
  "AtomicCompareExchangeWeak\0"
  "AtomicIIncrement\0"
  "AtomicIDecrement\0"
  "AtomicIAdd\0"
  "AtomicISub\0"
  "AtomicSMin\0"
  "AtomicUMin\0"
  "AtomicSMax\0"
  "AtomicUMax\0"
  "AtomicAnd\0"
  "AtomicOr\0"
  "AtomicXor\0"
  "Phi\0"
  "SPV_OPERAND_TYPE_LOOP_CONTROL\0"
  "LoopMerge\0"
  "SPV_OPERAND_TYPE_SELECTION_CONTROL\0"
  "SelectionMerge\0"
  "Label\0"
  "Branch\0"
  "BranchConditional\0"
  "SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER_ID\0"
  "Switch\0"
  "Kill\0"
  "Return\0"
  "ReturnValue\0"
  "Unreachable\0"
  "LifetimeStart\0"
  "LifetimeStop\0"
  "GroupAsyncCopy\0"
  "GroupWaitEvents\0"
  "GroupAll\0"
  "GroupAny\0"
  "GroupBroadcast\0"
  "SPV_OPERAND_TYPE_GROUP_OPERATION\0"
  "GroupIAdd\0"
  "GroupFAdd\0"
  "GroupFMin\0"
  "GroupUMin\0"
  "GroupSMin\0"
  "GroupFMax\0"
  "GroupUMax\0"
  "GroupSMax\0"
  "ReadPipe\0"
  "WritePipe\0"
  "ReservedReadPipe\0"
  "ReservedWritePipe\0"
  "ReserveReadPipePackets\0"
  "ReserveWritePipePackets\0"
  "CommitReadPipe\0"
  "CommitWritePipe\0"
  "IsValidReserveId\0"
  "GetNumPipePackets\0"
  "GetMaxPipePackets\0"
  "GroupReserveReadPipePackets\0"
  "GroupReserveWritePipePackets\0"
  "GroupCommitReadPipe\0"
  "GroupCommitWritePipe\0"
  "EnqueueMarker\0"
  "EnqueueKernel\0"
  "GetKernelNDrangeSubGroupCount\0"
  "GetKernelNDrangeMaxSubGroupSize\0"
  "GetKernelWorkGroupSize\0"
  "GetKernelPreferredWorkGroupSizeMultiple\0"
  "RetainEvent\0"
  "ReleaseEvent\0"
  "CreateUserEvent\0"
  "IsValidEvent\0"
  "SetUserEventStatus\0"
  "CaptureEventProfilingInfo\0"
  "GetDefaultQueue\0"
  "BuildNDRange\0"
  "ImageSparseSampleImplicitLod\0"
  "ImageSparseSampleExplicitLod\0"
  "ImageSparseSampleDrefImplicitLod\0"
  "ImageSparseSampleDrefExplicitLod\0"
  "ImageSparseSampleProjImplicitLod\0"
  "ImageSparseSampleProjExplicitLod\0"
  "ImageSparseSampleProjDrefImplicitLod\0"
  "ImageSparseSampleProjDrefExplicitLod\0"
  "ImageSparseFetch\0"
  "ImageSparseGather\0"
  "ImageSparseDrefGather\0"
  "ImageSparseTexelsResident\0"
  "NoLine\0"
  "AtomicFlagTestAndSet\0"
  "AtomicFlagClear\0"
  "ImageSparseRead\0"
  "SizeOf\0"
  "TypePipeStorage\0"
  "ConstantPipeStorage\0"
  "CreatePipeFromPipeStorage\0"
  "GetKernelLocalSizeForSubgroupCount\0"
  "GetKernelMaxNumSubgroups\0"
  "TypeNamedBarrier\0"
  "NamedBarrierInitialize\0"
  "MemoryNamedBarrier\0"
  "ModuleProcessed\0"
  "ExecutionModeId\0"
  "DecorateId\0"
  "GroupNonUniformElect\0"
  "GroupNonUniformAll\0"
  "GroupNonUniformAny\0"
  "GroupNonUniformAllEqual\0"
  "GroupNonUniformBroadcast\0"
  "GroupNonUniformBroadcastFirst\0"
  "GroupNonUniformInverseBallot\0"
  "GroupNonUniformBallotBitExtract\0"
  "GroupNonUniformBallotBitCount\0"
  "GroupNonUniformBallotFindLSB\0"
  "GroupNonUniformBallotFindMSB\0"
  "GroupNonUniformShuffleXor\0"
  "GroupNonUniformShuffleUp\0"
  "GroupNonUniformShuffleDown\0"
  "GroupNonUniformIAdd\0"
  "GroupNonUniformFAdd\0"
  "GroupNonUniformIMul\0"
  "GroupNonUniformFMul\0"
  "GroupNonUniformSMin\0"
  "GroupNonUniformUMin\0"
  "GroupNonUniformFMin\0"
  "GroupNonUniformSMax\0"
  "GroupNonUniformUMax\0"
  "GroupNonUniformFMax\0"
  "GroupNonUniformBitwiseAnd\0"
  "GroupNonUniformBitwiseOr\0"
  "GroupNonUniformBitwiseXor\0"
  "GroupNonUniformLogicalAnd\0"
  "GroupNonUniformLogicalOr\0"
  "GroupNonUniformLogicalXor\0"
  "GroupNonUniformQuadBroadcast\0"
  "GroupNonUniformQuadSwap\0"
  "CopyLogical\0"
  "PtrEqual\0"
  "PtrNotEqual\0"
  "PtrDiff\0"
  "ColorAttachmentReadEXT\0"
  "DepthAttachmentReadEXT\0"
  "StencilAttachmentReadEXT\0"
  "TypeTensorARM\0"
  "SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS\0"
  "TensorReadARM\0"
  "TensorWriteARM\0"
  "TensorQuerySizeARM\0"
  "GraphConstantARM\0"
  "GraphEntryPointARM\0"
  "GraphInputARM\0"
  "GraphSetOutputARM\0"
  "GraphEndARM\0"
  "TypeGraphARM\0"
  "TerminateInvocation\0"
  "TypeUntypedPointerKHR\0"
  "UntypedVariableKHR\0"
  "UntypedAccessChainKHR\0"
  "UntypedInBoundsAccessChainKHR\0"
  "SubgroupFirstInvocationKHR\0"
  "UntypedPtrAccessChainKHR\0"
  "UntypedInBoundsPtrAccessChainKHR\0"
  "UntypedArrayLengthKHR\0"
  "UntypedPrefetchKHR\0"
  "FmaKHR\0"
  "SubgroupAllKHR\0"
  "SubgroupAnyKHR\0"
  "SubgroupAllEqualKHR\0"
  "SubgroupReadInvocationKHR\0"
  "ExtInstWithForwardRefsKHR\0"
  "UntypedGroupAsyncCopyKHR\0"
  "TraceRayKHR\0"
  "ExecuteCallableKHR\0"
  "ConvertUToAccelerationStructureKHR\0"
  "IgnoreIntersectionKHR\0"
  "TerminateRayKHR\0"
  "SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT\0"
  "SDot\0"
  "SDotKHR\0"
  "UDot\0"
  "UDotKHR\0"
  "SUDot\0"
  "SUDotKHR\0"
  "SDotAccSat\0"
  "SDotAccSatKHR\0"
  "UDotAccSat\0"
  "UDotAccSatKHR\0"
  "SUDotAccSat\0"
  "SUDotAccSatKHR\0"
  "TypeCooperativeMatrixKHR\0"
  "CooperativeMatrixLoadKHR\0"
  "CooperativeMatrixStoreKHR\0"
  "SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS\0"
  "CooperativeMatrixMulAddKHR\0"
  "CooperativeMatrixLengthKHR\0"
  "ConstantCompositeReplicateEXT\0"
  "SpecConstantCompositeReplicateEXT\0"
  "CompositeConstructReplicateEXT\0"
  "TypeRayQueryKHR\0"
  "RayQueryInitializeKHR\0"
  "RayQueryTerminateKHR\0"
  "RayQueryGenerateIntersectionKHR\0"
  "RayQueryConfirmIntersectionKHR\0"
  "RayQueryProceedKHR\0"
  "RayQueryGetIntersectionTypeKHR\0"
  "ImageSampleWeightedQCOM\0"
  "ImageBoxFilterQCOM\0"
  "ImageBlockMatchSSDQCOM\0"
  "ImageBlockMatchSADQCOM\0"
  "BitCastArrayQCOM\0"
  "ImageBlockMatchWindowSSDQCOM\0"
  "ImageBlockMatchWindowSADQCOM\0"
  "ImageBlockMatchGatherSSDQCOM\0"
  "ImageBlockMatchGatherSADQCOM\0"
  "CompositeConstructCoopMatQCOM\0"
  "CompositeExtractCoopMatQCOM\0"
  "ExtractSubArrayQCOM\0"
  "ImageGatherQCOM\0"
  "GroupIAddNonUniformAMD\0"
  "GroupFAddNonUniformAMD\0"
  "GroupFMinNonUniformAMD\0"
  "GroupUMinNonUniformAMD\0"
  "GroupSMinNonUniformAMD\0"
  "GroupFMaxNonUniformAMD\0"
  "GroupUMaxNonUniformAMD\0"
  "GroupSMaxNonUniformAMD\0"
  "FragmentMaskFetchAMD\0"
  "FragmentFetchAMD\0"
  "ReadClockKHR\0"
  "AllocateNodePayloadsAMDX\0"
  "EnqueueNodePayloadsAMDX\0"
  "TypeNodePayloadArrayAMDX\0"
  "FinishWritingNodePayloadAMDX\0"
  "NodePayloadArrayLengthAMDX\0"
  "IsNodePayloadValidAMDX\0"
  "ConstantStringAMDX\0"
  "SpecConstantStringAMDX\0"
  "GroupNonUniformQuadAllKHR\0"
  "GroupNonUniformQuadAnyKHR\0"
  "TypeBufferEXT\0"
  "BufferPointerEXT\0"
  "UntypedImageTexelPointerEXT\0"
  "MemberDecorateIdEXT\0"
  "ConstantSizeOfEXT\0"
  "SpecConstantDataKHR\0"
  "PoisonKHR\0"
  "FreezeKHR\0"
  "HitObjectRecordHitMotionNV\0"
  "HitObjectRecordHitWithIndexMotionNV\0"
  "HitObjectRecordMissMotionNV\0"
  "HitObjectGetWorldToObjectNV\0"
  "HitObjectGetObjectToWorldNV\0"
  "HitObjectGetObjectRayDirectionNV\0"
  "HitObjectGetObjectRayOriginNV\0"
  "HitObjectTraceRayMotionNV\0"
  "HitObjectGetShaderRecordBufferHandleNV\0"
  "HitObjectGetShaderBindingTableRecordIndexNV\0"
  "HitObjectRecordEmptyNV\0"
  "HitObjectTraceRayNV\0"
  "HitObjectRecordHitNV\0"
  "HitObjectRecordHitWithIndexNV\0"
  "HitObjectRecordMissNV\0"
  "HitObjectExecuteShaderNV\0"
  "HitObjectGetCurrentTimeNV\0"
  "HitObjectGetAttributesNV\0"
  "HitObjectGetHitKindNV\0"
  "HitObjectGetPrimitiveIndexNV\0"
  "HitObjectGetGeometryIndexNV\0"
  "HitObjectGetInstanceIdNV\0"
  "HitObjectGetInstanceCustomIndexNV\0"
  "HitObjectGetWorldRayDirectionNV\0"
  "HitObjectGetWorldRayOriginNV\0"
  "HitObjectGetRayTMaxNV\0"
  "HitObjectGetRayTMinNV\0"
  "HitObjectIsEmptyNV\0"
  "HitObjectIsHitNV\0"
  "HitObjectIsMissNV\0"
  "ReorderThreadWithHitObjectNV\0"
  "ReorderThreadWithHintNV\0"
  "TypeHitObjectNV\0"
  "ImageSampleFootprintNV\0"
  "TypeVectorIdEXT\0"
  "TypeCooperativeVectorNV\0"
  "CooperativeVectorMatrixMulNV\0"
  "CooperativeVectorOuterProductAccumulateNV\0"
  "CooperativeVectorReduceSumAccumulateNV\0"
  "CooperativeVectorMatrixMulAddNV\0"
  "CooperativeMatrixConvertNV\0"
  "EmitMeshTasksEXT\0"
  "SetMeshOutputsEXT\0"
  "GroupNonUniformPartitionEXT\0"
  "GroupNonUniformPartitionNV\0"
  "WritePackedPrimitiveIndices4x8NV\0"
  "FetchMicroTriangleVertexPositionNV\0"
  "FetchMicroTriangleVertexBarycentricNV\0"
  "CooperativeVectorLoadNV\0"
  "CooperativeVectorStoreNV\0"
  "HitObjectRecordFromQueryEXT\0"
  "HitObjectRecordMissEXT\0"
  "HitObjectRecordMissMotionEXT\0"
  "HitObjectGetIntersectionTriangleVertexPositionsEXT\0"
  "HitObjectGetRayFlagsEXT\0"
  "HitObjectSetShaderBindingTableRecordIndexEXT\0"
  "HitObjectReorderExecuteShaderEXT\0"
  "HitObjectTraceReorderExecuteEXT\0"
  "HitObjectTraceMotionReorderExecuteEXT\0"
  "TypeHitObjectEXT\0"
  "ReorderThreadWithHintEXT\0"
  "ReorderThreadWithHitObjectEXT\0"
  "HitObjectTraceRayEXT\0"
  "HitObjectTraceRayMotionEXT\0"
  "HitObjectRecordEmptyEXT\0"
  "HitObjectExecuteShaderEXT\0"
  "HitObjectGetCurrentTimeEXT\0"
  "HitObjectGetAttributesEXT\0"
  "HitObjectGetHitKindEXT\0"
  "HitObjectGetPrimitiveIndexEXT\0"
  "HitObjectGetGeometryIndexEXT\0"
  "HitObjectGetInstanceIdEXT\0"
  "HitObjectGetInstanceCustomIndexEXT\0"
  "HitObjectGetObjectRayOriginEXT\0"
  "HitObjectGetObjectRayDirectionEXT\0"
  "HitObjectGetWorldRayDirectionEXT\0"
  "HitObjectGetWorldRayOriginEXT\0"
  "HitObjectGetObjectToWorldEXT\0"
  "HitObjectGetWorldToObjectEXT\0"
  "HitObjectGetRayTMaxEXT\0"
  "ReportIntersectionKHR\0"
  "ReportIntersectionNV\0"
  "IgnoreIntersectionNV\0"
  "TerminateRayNV\0"
  "TraceNV\0"
  "TraceMotionNV\0"
  "TraceRayMotionNV\0"
  "RayQueryGetIntersectionTriangleVertexPositionsKHR\0"
  "TypeAccelerationStructureKHR\0"
  "TypeAccelerationStructureNV\0"
  "ExecuteCallableNV\0"
  "RayQueryGetIntersectionClusterIdNV\0"
  "RayQueryGetClusterIdNV\0"
  "HitObjectGetClusterIdNV\0"
  "HitObjectGetRayTMinEXT\0"
  "HitObjectGetShaderBindingTableRecordIndexEXT\0"
  "HitObjectGetShaderRecordBufferHandleEXT\0"
  "HitObjectIsEmptyEXT\0"
  "HitObjectIsHitEXT\0"
  "HitObjectIsMissEXT\0"
  "TypeCooperativeMatrixNV\0"
  "CooperativeMatrixLoadNV\0"
  "CooperativeMatrixStoreNV\0"
  "CooperativeMatrixMulAddNV\0"
  "CooperativeMatrixLengthNV\0"
  "BeginInvocationInterlockEXT\0"
  "EndInvocationInterlockEXT\0"
  "SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE\0"
  "CooperativeMatrixReduceNV\0"
  "SPV_OPERAND_TYPE_MEMORY_ACCESS\0"
  "SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS\0"
  "CooperativeMatrixLoadTensorNV\0"
  "CooperativeMatrixStoreTensorNV\0"
  "CooperativeMatrixPerElementOpNV\0"
  "TypeTensorLayoutNV\0"
  "TypeTensorViewNV\0"
  "CreateTensorLayoutNV\0"
  "TensorLayoutSetDimensionNV\0"
  "TensorLayoutSetStrideNV\0"
  "TensorLayoutSliceNV\0"
  "TensorLayoutSetClampValueNV\0"
  "CreateTensorViewNV\0"
  "TensorViewSetDimensionNV\0"
  "TensorViewSetStrideNV\0"
  "IsHelperInvocationEXT\0"
  "TensorViewSetClipNV\0"
  "TensorLayoutSetBlockSizeNV\0"
  "CooperativeMatrixTransposeNV\0"
  "ConvertUToImageNV\0"
  "ConvertUToSamplerNV\0"
  "ConvertImageToUNV\0"
  "ConvertSamplerToUNV\0"
  "ConvertUToSampledImageNV\0"
  "ConvertSampledImageToUNV\0"
  "SamplerImageAddressingModeNV\0"
  "SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS\0"
  "RawAccessChainNV\0"
  "RayQueryGetIntersectionSpherePositionNV\0"
  "RayQueryGetIntersectionSphereRadiusNV\0"
  "RayQueryGetIntersectionLSSPositionsNV\0"
  "RayQueryGetIntersectionLSSRadiiNV\0"
  "RayQueryGetIntersectionLSSHitValueNV\0"
  "HitObjectGetSpherePositionNV\0"
  "HitObjectGetSphereRadiusNV\0"
  "HitObjectGetLSSPositionsNV\0"
  "HitObjectGetLSSRadiiNV\0"
  "HitObjectIsSphereHitNV\0"
  "HitObjectIsLSSHitNV\0"
  "RayQueryIsSphereHitNV\0"
  "RayQueryIsLSSHitNV\0"
  "SubgroupShuffleDownINTEL\0"
  "SubgroupShuffleUpINTEL\0"
  "SubgroupShuffleXorINTEL\0"
  "SubgroupBlockReadINTEL\0"
  "SubgroupBlockWriteINTEL\0"
  "SubgroupImageBlockReadINTEL\0"
  "SubgroupImageBlockWriteINTEL\0"
  "SubgroupImageMediaBlockReadINTEL\0"
  "SubgroupImageMediaBlockWriteINTEL\0"
  "UCountLeadingZerosINTEL\0"
  "UCountTrailingZerosINTEL\0"
  "AbsISubINTEL\0"
  "AbsUSubINTEL\0"
  "IAddSatINTEL\0"
  "UAddSatINTEL\0"
  "IAverageINTEL\0"
  "UAverageINTEL\0"
  "IAverageRoundedINTEL\0"
  "UAverageRoundedINTEL\0"
  "ISubSatINTEL\0"
  "USubSatINTEL\0"
  "IMul32x16INTEL\0"
  "UMul32x16INTEL\0"
  "ConstantFunctionPointerINTEL\0"
  "FunctionPointerCallINTEL\0"
  "AsmTargetINTEL\0"
  "AsmCallINTEL\0"
  "AtomicFMinEXT\0"
  "AtomicFMaxEXT\0"
  "AssumeTrueKHR\0"
  "ExpectKHR\0"
  "DecorateString\0"
  "DecorateStringGOOGLE\0"
  "MemberDecorateString\0"
  "MemberDecorateStringGOOGLE\0"
  "VmeImageINTEL\0"
  "TypeVmeImageINTEL\0"
  "TypeAvcImePayloadINTEL\0"
  "TypeAvcRefPayloadINTEL\0"
  "TypeAvcSicPayloadINTEL\0"
  "TypeAvcMcePayloadINTEL\0"
  "TypeAvcMceResultINTEL\0"
  "TypeAvcImeResultINTEL\0"
  "TypeAvcImeResultSingleReferenceStreamoutINTEL\0"
  "TypeAvcImeResultDualReferenceStreamoutINTEL\0"
  "TypeAvcImeSingleReferenceStreaminINTEL\0"
  "TypeAvcImeDualReferenceStreaminINTEL\0"
  "TypeAvcRefResultINTEL\0"
  "TypeAvcSicResultINTEL\0"
  "SubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL\0"
  "SubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL\0"
  "SubgroupAvcMceGetDefaultInterShapePenaltyINTEL\0"
  "SubgroupAvcMceSetInterShapePenaltyINTEL\0"
  "SubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL\0"
  "SubgroupAvcMceSetInterDirectionPenaltyINTEL\0"
  "SubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL\0"
  "SubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL\0"
  "SubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL\0"
  "SubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL\0"
  "SubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL\0"
  "SubgroupAvcMceSetMotionVectorCostFunctionINTEL\0"
  "SubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL\0"
  "SubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL\0"
  "SubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL\0"
  "SubgroupAvcMceSetAcOnlyHaarINTEL\0"
  "SubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL\0"
  "SubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL\0"
  "SubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL\0"
  "SubgroupAvcMceConvertToImePayloadINTEL\0"
  "SubgroupAvcMceConvertToImeResultINTEL\0"
  "SubgroupAvcMceConvertToRefPayloadINTEL\0"
  "SubgroupAvcMceConvertToRefResultINTEL\0"
  "SubgroupAvcMceConvertToSicPayloadINTEL\0"
  "SubgroupAvcMceConvertToSicResultINTEL\0"
  "SubgroupAvcMceGetMotionVectorsINTEL\0"
  "SubgroupAvcMceGetInterDistortionsINTEL\0"
  "SubgroupAvcMceGetBestInterDistortionsINTEL\0"
  "SubgroupAvcMceGetInterMajorShapeINTEL\0"
  "SubgroupAvcMceGetInterMinorShapeINTEL\0"
  "SubgroupAvcMceGetInterDirectionsINTEL\0"
  "SubgroupAvcMceGetInterMotionVectorCountINTEL\0"
  "SubgroupAvcMceGetInterReferenceIdsINTEL\0"
  "SubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL\0"
  "SubgroupAvcImeInitializeINTEL\0"
  "SubgroupAvcImeSetSingleReferenceINTEL\0"
  "SubgroupAvcImeSetDualReferenceINTEL\0"
  "SubgroupAvcImeRefWindowSizeINTEL\0"
  "SubgroupAvcImeAdjustRefOffsetINTEL\0"
  "SubgroupAvcImeConvertToMcePayloadINTEL\0"
  "SubgroupAvcImeSetMaxMotionVectorCountINTEL\0"
  "SubgroupAvcImeSetUnidirectionalMixDisableINTEL\0"
  "SubgroupAvcImeSetEarlySearchTerminationThresholdINTEL\0"
  "SubgroupAvcImeSetWeightedSadINTEL\0"
  "SubgroupAvcImeEvaluateWithSingleReferenceINTEL\0"
  "SubgroupAvcImeEvaluateWithDualReferenceINTEL\0"
  "SubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL\0"
  "SubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL\0"
  "SubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL\0"
  "SubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL\0"
  "SubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL\0"
  "SubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL\0"
  "SubgroupAvcImeConvertToMceResultINTEL\0"
  "SubgroupAvcImeGetSingleReferenceStreaminINTEL\0"
  "SubgroupAvcImeGetDualReferenceStreaminINTEL\0"
  "SubgroupAvcImeStripSingleReferenceStreamoutINTEL\0"
  "SubgroupAvcImeStripDualReferenceStreamoutINTEL\0"
  "SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL\0"
  "SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL\0"
  "SubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL\0"
  "SubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL\0"
  "SubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL\0"
  "SubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL\0"
  "SubgroupAvcImeGetBorderReachedINTEL\0"
  "SubgroupAvcImeGetTruncatedSearchIndicationINTEL\0"
  "SubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL\0"
  "SubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL\0"
  "SubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL\0"
  "SubgroupAvcFmeInitializeINTEL\0"
  "SubgroupAvcBmeInitializeINTEL\0"
  "SubgroupAvcRefConvertToMcePayloadINTEL\0"
  "SubgroupAvcRefSetBidirectionalMixDisableINTEL\0"
  "SubgroupAvcRefSetBilinearFilterEnableINTEL\0"
  "SubgroupAvcRefEvaluateWithSingleReferenceINTEL\0"
  "SubgroupAvcRefEvaluateWithDualReferenceINTEL\0"
  "SubgroupAvcRefEvaluateWithMultiReferenceINTEL\0"
  "SubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL\0"
  "SubgroupAvcRefConvertToMceResultINTEL\0"
  "SubgroupAvcSicInitializeINTEL\0"
  "SubgroupAvcSicConfigureSkcINTEL\0"
  "SubgroupAvcSicConfigureIpeLumaINTEL\0"
  "SubgroupAvcSicConfigureIpeLumaChromaINTEL\0"
  "SubgroupAvcSicGetMotionVectorMaskINTEL\0"
  "SubgroupAvcSicConvertToMcePayloadINTEL\0"
  "SubgroupAvcSicSetIntraLumaShapePenaltyINTEL\0"
  "SubgroupAvcSicSetIntraLumaModeCostFunctionINTEL\0"
  "SubgroupAvcSicSetIntraChromaModeCostFunctionINTEL\0"
  "SubgroupAvcSicSetBilinearFilterEnableINTEL\0"
  "SubgroupAvcSicSetSkcForwardTransformEnableINTEL\0"
  "SubgroupAvcSicSetBlockBasedRawSkipSadINTEL\0"
  "SubgroupAvcSicEvaluateIpeINTEL\0"
  "SubgroupAvcSicEvaluateWithSingleReferenceINTEL\0"
  "SubgroupAvcSicEvaluateWithDualReferenceINTEL\0"
  "SubgroupAvcSicEvaluateWithMultiReferenceINTEL\0"
  "SubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL\0"
  "SubgroupAvcSicConvertToMceResultINTEL\0"
  "SubgroupAvcSicGetIpeLumaShapeINTEL\0"
  "SubgroupAvcSicGetBestIpeLumaDistortionINTEL\0"
  "SubgroupAvcSicGetBestIpeChromaDistortionINTEL\0"
  "SubgroupAvcSicGetPackedIpeLumaModesINTEL\0"
  "SubgroupAvcSicGetIpeChromaModeINTEL\0"
  "SubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL\0"
  "SubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL\0"
  "SubgroupAvcSicGetInterRawSadsINTEL\0"
  "SaveMemoryINTEL\0"
  "RestoreMemoryINTEL\0"
  "ArbitraryFloatSinCosPiALTERA\0"
  "ArbitraryFloatSinCosPiINTEL\0"
  "ArbitraryFloatCastALTERA\0"
  "ArbitraryFloatCastINTEL\0"
  "ArbitraryFloatCastFromIntALTERA\0"
  "ArbitraryFloatCastFromIntINTEL\0"
  "ArbitraryFloatCastToIntALTERA\0"
  "ArbitraryFloatCastToIntINTEL\0"
  "ArbitraryFloatAddALTERA\0"
  "ArbitraryFloatAddINTEL\0"
  "ArbitraryFloatSubALTERA\0"
  "ArbitraryFloatSubINTEL\0"
  "ArbitraryFloatMulALTERA\0"
  "ArbitraryFloatMulINTEL\0"
  "ArbitraryFloatDivALTERA\0"
  "ArbitraryFloatDivINTEL\0"
  "ArbitraryFloatGTALTERA\0"
  "ArbitraryFloatGTINTEL\0"
  "ArbitraryFloatGEALTERA\0"
  "ArbitraryFloatGEINTEL\0"
  "ArbitraryFloatLTALTERA\0"
  "ArbitraryFloatLTINTEL\0"
  "ArbitraryFloatLEALTERA\0"
  "ArbitraryFloatLEINTEL\0"
  "ArbitraryFloatEQALTERA\0"
  "ArbitraryFloatEQINTEL\0"
  "ArbitraryFloatRecipALTERA\0"
  "ArbitraryFloatRecipINTEL\0"
  "ArbitraryFloatRSqrtALTERA\0"
  "ArbitraryFloatRSqrtINTEL\0"
  "ArbitraryFloatCbrtALTERA\0"
  "ArbitraryFloatCbrtINTEL\0"
  "ArbitraryFloatHypotALTERA\0"
  "ArbitraryFloatHypotINTEL\0"
  "ArbitraryFloatSqrtALTERA\0"
  "ArbitraryFloatSqrtINTEL\0"
  "ArbitraryFloatLogINTEL\0"
  "ArbitraryFloatLog2INTEL\0"
  "ArbitraryFloatLog10INTEL\0"
  "ArbitraryFloatLog1pINTEL\0"
  "ArbitraryFloatExpINTEL\0"
  "ArbitraryFloatExp2INTEL\0"
  "ArbitraryFloatExp10INTEL\0"
  "ArbitraryFloatExpm1INTEL\0"
  "ArbitraryFloatSinINTEL\0"
  "ArbitraryFloatCosINTEL\0"
  "ArbitraryFloatSinCosINTEL\0"
  "ArbitraryFloatSinPiINTEL\0"
  "ArbitraryFloatCosPiINTEL\0"
  "ArbitraryFloatASinINTEL\0"
  "ArbitraryFloatASinPiINTEL\0"
  "ArbitraryFloatACosINTEL\0"
  "ArbitraryFloatACosPiINTEL\0"
  "ArbitraryFloatATanINTEL\0"
  "ArbitraryFloatATanPiINTEL\0"
  "ArbitraryFloatATan2INTEL\0"
  "ArbitraryFloatPowINTEL\0"
  "ArbitraryFloatPowRINTEL\0"
  "ArbitraryFloatPowNINTEL\0"
  "LoopControlINTEL\0"
  "AliasDomainDeclINTEL\0"
  "AliasScopeDeclINTEL\0"
  "AliasScopeListDeclINTEL\0"
  "FixedSqrtALTERA\0"
  "FixedSqrtINTEL\0"
  "FixedRecipALTERA\0"
  "FixedRecipINTEL\0"
  "FixedRsqrtALTERA\0"
  "FixedRsqrtINTEL\0"
  "FixedSinALTERA\0"
  "FixedSinINTEL\0"
  "FixedCosALTERA\0"
  "FixedCosINTEL\0"
  "FixedSinCosALTERA\0"
  "FixedSinCosINTEL\0"
  "FixedSinPiALTERA\0"
  "FixedSinPiINTEL\0"
  "FixedCosPiALTERA\0"
  "FixedCosPiINTEL\0"
  "FixedSinCosPiALTERA\0"
  "FixedSinCosPiINTEL\0"
  "FixedLogALTERA\0"
  "FixedLogINTEL\0"
  "FixedExpALTERA\0"
  "FixedExpINTEL\0"
  "PtrCastToCrossWorkgroupALTERA\0"
  "PtrCastToCrossWorkgroupINTEL\0"
  "CrossWorkgroupCastToPtrALTERA\0"
  "CrossWorkgroupCastToPtrINTEL\0"
  "ReadPipeBlockingALTERA\0"
  "ReadPipeBlockingINTEL\0"
  "WritePipeBlockingALTERA\0"
  "WritePipeBlockingINTEL\0"
  "RayQueryGetRayTMinKHR\0"
  "RayQueryGetRayFlagsKHR\0"
  "RayQueryGetIntersectionTKHR\0"
  "RayQueryGetIntersectionInstanceCustomIndexKHR\0"
  "RayQueryGetIntersectionInstanceIdKHR\0"
  "RayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR\0"
  "RayQueryGetIntersectionGeometryIndexKHR\0"
  "RayQueryGetIntersectionPrimitiveIndexKHR\0"
  "RayQueryGetIntersectionBarycentricsKHR\0"
  "RayQueryGetIntersectionFrontFaceKHR\0"
  "RayQueryGetIntersectionCandidateAABBOpaqueKHR\0"
  "RayQueryGetIntersectionObjectRayDirectionKHR\0"
  "RayQueryGetIntersectionObjectRayOriginKHR\0"
  "RayQueryGetWorldRayDirectionKHR\0"
  "RayQueryGetWorldRayOriginKHR\0"
  "RayQueryGetIntersectionObjectToWorldKHR\0"
  "RayQueryGetIntersectionWorldToObjectKHR\0"
  "AtomicFAddEXT\0"
  "TypeBufferSurfaceINTEL\0"
  "TypeStructContinuedINTEL\0"
  "ConstantCompositeContinuedINTEL\0"
  "SpecConstantCompositeContinuedINTEL\0"
  "CompositeConstructContinuedINTEL\0"
  "ConvertFToBF16INTEL\0"
  "ConvertBF16ToFINTEL\0"
  "ControlBarrierArriveEXT\0"
  "ControlBarrierArriveINTEL\0"
  "ControlBarrierWaitEXT\0"
  "ControlBarrierWaitINTEL\0"
  "TaskSequenceCreateALTERA\0"
  "TaskSequenceCreateINTEL\0"
  "TaskSequenceAsyncALTERA\0"
  "TaskSequenceAsyncINTEL\0"
  "TaskSequenceGetALTERA\0"
  "TaskSequenceGetINTEL\0"
  "TaskSequenceReleaseALTERA\0"
  "TaskSequenceReleaseINTEL\0"
  "TypeTaskSequenceALTERA\0"
  "TypeTaskSequenceINTEL\0"
  "SubgroupBlockPrefetchINTEL\0"
  "Subgroup2DBlockLoadINTEL\0"
  "Subgroup2DBlockLoadTransformINTEL\0"
  "Subgroup2DBlockLoadTransposeINTEL\0"
  "Subgroup2DBlockPrefetchINTEL\0"
  "Subgroup2DBlockStoreINTEL\0"
  "SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS\0"
  "BitwiseFunctionINTEL\0"
  "ConditionalExtensionINTEL\0"
  "ConditionalEntryPointINTEL\0"
  "ConditionalCapabilityINTEL\0"
  "SpecConstantTargetINTEL\0"
  "SpecConstantArchitectureINTEL\0"
  "SPV_OPERAND_TYPE_VARIABLE_CAPABILITY\0"
  "SpecConstantCapabilitiesINTEL\0"
  "ConditionalCopyObjectINTEL\0"
  "PredicatedLoadINTEL\0"
  "PredicatedStoreINTEL\0"
  "GroupIMulKHR\0"
  "GroupFMulKHR\0"
  "GroupBitwiseAndKHR\0"
  "GroupBitwiseOrKHR\0"
  "GroupBitwiseXorKHR\0"
  "GroupLogicalAndKHR\0"
  "GroupLogicalOrKHR\0"
  "GroupLogicalXorKHR\0"
  "RoundFToTF32INTEL\0"
  "MaskedGatherINTEL\0"
  "MaskedScatterINTEL\0"
  "ConvertHandleToImageINTEL\0"
  "ConvertHandleToSamplerINTEL\0"
  "ConvertHandleToSampledImageINTEL\0"
  "FDot2MixAcc32VALVE\0"
  "FDot2MixAcc16VALVE\0"
  "FDot4MixAcc32VALVE\0"
  "CALL\0"
  "MIN_SAD\0"
  "MIN_SAD_COST\0"
  "RAW_SAD\0"
  "DebugInfoNone\0"
  "DebugCompilationUnit\0"
  "SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING\0"
  "DebugTypeBasic\0"
  "SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS\0"
  "DebugTypePointer\0"
  "SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER\0"
  "DebugTypeQualifier\0"
  "DebugTypeArray\0"
  "DebugTypeVector\0"
  "DebugTypedef\0"
  "DebugTypeFunction\0"
  "DebugTypeEnum\0"
  "SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE\0"
  "DebugTypeComposite\0"
  "DebugTypeMember\0"
  "DebugTypeInheritance\0"
  "DebugTypePtrToMember\0"
  "DebugTypeTemplate\0"
  "DebugTypeTemplateParameter\0"
  "DebugTypeTemplateTemplateParameter\0"
  "DebugTypeTemplateParameterPack\0"
  "DebugGlobalVariable\0"
  "DebugFunctionDeclaration\0"
  "DebugFunction\0"
  "DebugLexicalBlock\0"
  "DebugLexicalBlockDiscriminator\0"
  "DebugScope\0"
  "DebugNoScope\0"
  "DebugInlinedAt\0"
  "SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER\0"
  "DebugLocalVariable\0"
  "DebugInlinedVariable\0"
  "DebugDeclare\0"
  "DebugValue\0"
  "SPV_OPERAND_TYPE_DEBUG_OPERATION\0"
  "DebugOperation\0"
  "DebugExpression\0"
  "DebugMacroDef\0"
  "DebugMacroUndef\0"
  "Round\0"
  "RoundEven\0"
  "Trunc\0"
  "FAbs\0"
  "SAbs\0"
  "FSign\0"
  "SSign\0"
  "Floor\0"
  "Ceil\0"
  "Fract\0"
  "Radians\0"
  "Degrees\0"
  "Sin\0"
  "Cos\0"
  "Tan\0"
  "Asin\0"
  "Acos\0"
  "Atan\0"
  "Sinh\0"
  "Cosh\0"
  "Tanh\0"
  "Asinh\0"
  "Acosh\0"
  "Atanh\0"
  "Atan2\0"
  "Pow\0"
  "Exp\0"
  "Log\0"
  "Exp2\0"
  "Log2\0"
  "Sqrt\0"
  "InverseSqrt\0"
  "Determinant\0"
  "MatrixInverse\0"
  "Modf\0"
  "ModfStruct\0"
  "FMin\0"
  "UMin\0"
  "SMin\0"
  "FMax\0"
  "UMax\0"
  "SMax\0"
  "FClamp\0"
  "UClamp\0"
  "SClamp\0"
  "FMix\0"
  "IMix\0"
  "Step\0"
  "SmoothStep\0"
  "Fma\0"
  "Frexp\0"
  "FrexpStruct\0"
  "Ldexp\0"
  "PackSnorm4x8\0"
  "PackUnorm4x8\0"
  "PackSnorm2x16\0"
  "PackUnorm2x16\0"
  "PackHalf2x16\0"
  "PackDouble2x32\0"
  "UnpackSnorm2x16\0"
  "UnpackUnorm2x16\0"
  "UnpackHalf2x16\0"
  "UnpackSnorm4x8\0"
  "UnpackUnorm4x8\0"
  "UnpackDouble2x32\0"
  "Length\0"
  "Distance\0"
  "Cross\0"
  "Normalize\0"
  "FaceForward\0"
  "Reflect\0"
  "Refract\0"
  "FindILsb\0"
  "FindSMsb\0"
  "FindUMsb\0"
  "InterpolateAtCentroid\0"
  "InterpolateAtSample\0"
  "InterpolateAtOffset\0"
  "NMin\0"
  "NMax\0"
  "NClamp\0"
  "ArgumentInfo\0"
  "ArgumentStorageBuffer\0"
  "ArgumentUniform\0"
  "ArgumentPodStorageBuffer\0"
  "ArgumentPodUniform\0"
  "ArgumentPodPushConstant\0"
  "ArgumentSampledImage\0"
  "ArgumentStorageImage\0"
  "ArgumentSampler\0"
  "ArgumentWorkgroup\0"
  "SpecConstantWorkgroupSize\0"
  "SpecConstantGlobalOffset\0"
  "SpecConstantWorkDim\0"
  "PushConstantGlobalOffset\0"
  "PushConstantEnqueuedLocalSize\0"
  "PushConstantGlobalSize\0"
  "PushConstantRegionOffset\0"
  "PushConstantNumWorkgroups\0"
  "PushConstantRegionGroupOffset\0"
  "ConstantDataStorageBuffer\0"
  "ConstantDataUniform\0"
  "PropertyRequiredWorkgroupSize\0"
  "SpecConstantSubgroupMaxSize\0"
  "ArgumentPointerPushConstant\0"
  "ArgumentPointerUniform\0"
  "ProgramScopeVariablesStorageBuffer\0"
  "ProgramScopeVariablePointerRelocation\0"
  "ImageArgumentInfoChannelOrderPushConstant\0"
  "ImageArgumentInfoChannelDataTypePushConstant\0"
  "ImageArgumentInfoChannelOrderUniform\0"
  "ImageArgumentInfoChannelDataTypeUniform\0"
  "ArgumentStorageTexelBuffer\0"
  "ArgumentUniformTexelBuffer\0"
  "ConstantDataPointerPushConstant\0"
  "ProgramScopeVariablePointerPushConstant\0"
  "PrintfInfo\0"
  "PrintfBufferStorageBuffer\0"
  "PrintfBufferPointerPushConstant\0"
  "NormalizedSamplerMaskPushConstant\0"
  "WorkgroupVariableSize\0"
  "DebugGraph\0"
  "DebugTensor\0"
  "DebugImportedEntity\0"
  "DebugSource\0"
  "DebugFunctionDefinition\0"
  "DebugSourceContinued\0"
  "DebugLine\0"
  "DebugNoLine\0"
  "DebugBuildIdentifier\0"
  "DebugStoragePath\0"
  "DebugEntryPoint\0"
  "DebugTypeMatrix\0"
  "DebugTypeVectorIdEXT\0"
  "DebugTypeCooperativeMatrixKHR\0"
  "Configuration\0"
  "StartCounter\0"
  "StopCounter\0"
  "PushConstants\0"
  "SpecializationMapEntry\0"
  "DescriptorSetBuffer\0"
  "DescriptorSetImage\0"
  "DescriptorSetSampler\0"
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING\0"
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS\0"
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER\0"
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE\0"
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION\0"
  "SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY\0"
  "DebugModuleINTEL\0"
  "acos\0"
  "acosh\0"
  "acospi\0"
  "asin\0"
  "asinh\0"
  "asinpi\0"
  "atan\0"
  "atan2\0"
  "atanh\0"
  "atanpi\0"
  "atan2pi\0"
  "cbrt\0"
  "ceil\0"
  "copysign\0"
  "cos\0"
  "cosh\0"
  "cospi\0"
  "erfc\0"
  "erf\0"
  "exp\0"
  "exp2\0"
  "exp10\0"
  "expm1\0"
  "fabs\0"
  "fdim\0"
  "floor\0"
  "fma\0"
  "fmax\0"
  "fmin\0"
  "fmod\0"
  "fract\0"
  "frexp\0"
  "hypot\0"
  "ilogb\0"
  "ldexp\0"
  "lgamma\0"
  "lgamma_r\0"
  "log\0"
  "log2\0"
  "log10\0"
  "log1p\0"
  "logb\0"
  "mad\0"
  "maxmag\0"
  "minmag\0"
  "modf\0"
  "nan\0"
  "nextafter\0"
  "pow\0"
  "pown\0"
  "powr\0"
  "remainder\0"
  "remquo\0"
  "rint\0"
  "rootn\0"
  "round\0"
  "rsqrt\0"
  "sin\0"
  "sincos\0"
  "sinh\0"
  "sinpi\0"
  "sqrt\0"
  "tan\0"
  "tanh\0"
  "tanpi\0"
  "tgamma\0"
  "trunc\0"
  "half_cos\0"
  "half_divide\0"
  "half_exp\0"
  "half_exp2\0"
  "half_exp10\0"
  "half_log\0"
  "half_log2\0"
  "half_log10\0"
  "half_powr\0"
  "half_recip\0"
  "half_rsqrt\0"
  "half_sin\0"
  "half_sqrt\0"
  "half_tan\0"
  "native_cos\0"
  "native_divide\0"
  "native_exp\0"
  "native_exp2\0"
  "native_exp10\0"
  "native_log\0"
  "native_log2\0"
  "native_log10\0"
  "native_powr\0"
  "native_recip\0"
  "native_rsqrt\0"
  "native_sin\0"
  "native_sqrt\0"
  "native_tan\0"
  "fclamp\0"
  "degrees\0"
  "fmax_common\0"
  "fmin_common\0"
  "mix\0"
  "radians\0"
  "step\0"
  "smoothstep\0"
  "sign\0"
  "cross\0"
  "distance\0"
  "length\0"
  "normalize\0"
  "fast_distance\0"
  "fast_length\0"
  "fast_normalize\0"
  "s_abs\0"
  "s_abs_diff\0"
  "s_add_sat\0"
  "u_add_sat\0"
  "s_hadd\0"
  "u_hadd\0"
  "s_rhadd\0"
  "u_rhadd\0"
  "s_clamp\0"
  "u_clamp\0"
  "clz\0"
  "ctz\0"
  "s_mad_hi\0"
  "u_mad_sat\0"
  "s_mad_sat\0"
  "s_max\0"
  "u_max\0"
  "s_min\0"
  "u_min\0"
  "s_mul_hi\0"
  "rotate\0"
  "s_sub_sat\0"
  "u_sub_sat\0"
  "u_upsample\0"
  "s_upsample\0"
  "popcount\0"
  "s_mad24\0"
  "u_mad24\0"
  "s_mul24\0"
  "u_mul24\0"
  "vloadn\0"
  "vstoren\0"
  "vload_half\0"
  "vload_halfn\0"
  "vstore_half\0"
  "vstore_half_r\0"
  "vstore_halfn\0"
  "vstore_halfn_r\0"
  "vloada_halfn\0"
  "vstorea_halfn\0"
  "vstorea_halfn_r\0"
  "shuffle\0"
  "shuffle2\0"
  "printf\0"
  "prefetch\0"
  "bitselect\0"
  "select\0"
  "u_abs\0"
  "u_abs_diff\0"
  "u_mul_hi\0"
  "u_mad_hi\0"
  "CubeFaceIndexAMD\0"
  "CubeFaceCoordAMD\0"
  "TimeAMD\0"
  "SwizzleInvocationsAMD\0"
  "SwizzleInvocationsMaskedAMD\0"
  "WriteInvocationAMD\0"
  "MbcntAMD\0"
  "InterpolateAtVertexAMD\0"
  "FMin3AMD\0"
  "UMin3AMD\0"
  "SMin3AMD\0"
  "FMax3AMD\0"
  "UMax3AMD\0"
  "SMax3AMD\0"
  "FMid3AMD\0"
  "UMid3AMD\0"
  "SMid3AMD\0"
  "ARGMAX\0"
  "AVG_POOL2D\0"
  "CONV2D\0"
  "CONV3D\0"
  "DEPTHWISE_CONV2D\0"
  "FFT2D\0"
  "MATMUL\0"
  "MAX_POOL2D\0"
  "RFFT2D\0"
  "TRANSPOSE_CONV2D\0"
  "CLAMP\0"
  "ERF\0"
  "SIGMOID\0"
  "TANH\0"
  "ADD\0"
  "ARITHMETIC_RIGHT_SHIFT\0"
  "BITWISE_AND\0"
  "BITWISE_OR\0"
  "BITWISE_XOR\0"
  "INTDIV\0"
  "LOGICAL_AND\0"
  "LOGICAL_LEFT_SHIFT\0"
  "LOGICAL_RIGHT_SHIFT\0"
  "LOGICAL_OR\0"
  "LOGICAL_XOR\0"
  "MAXIMUM\0"
  "MINIMUM\0"
  "MUL\0"
  "POW\0"
  "SUB\0"
  "TABLE\0"
  "ABS\0"
  "BITWISE_NOT\0"
  "CEIL\0"
  "CLZ\0"
  "COS\0"
  "EXP\0"
  "FLOOR\0"
  "LOG\0"
  "LOGICAL_NOT\0"
  "NEGATE\0"
  "RECIPROCAL\0"
  "RSQRT\0"
  "SIN\0"
  "SELECT\0"
  "EQUAL\0"
  "GREATER\0"
  "GREATER_EQUAL\0"
  "REDUCE_ALL\0"
  "REDUCE_ANY\0"
  "REDUCE_MAX\0"
  "REDUCE_MIN\0"
  "REDUCE_PRODUCT\0"
  "REDUCE_SUM\0"
  "CONCAT\0"
  "PAD\0"
  "RESHAPE\0"
  "REVERSE\0"
  "SLICE\0"
  "TILE\0"
  "TRANSPOSE\0"
  "GATHER\0"
  "SCATTER\0"
  "RESIZE\0"
  "CAST\0"
  "RESCALE\0"
;






static const IndexRange kAliasSpans[] = {
  IR(5918, 27),
  IR(6829, 18),
  IR(6884, 18),
  IR(6917, 18),
  IR(6950, 18),
  IR(6983, 18),
  IR(7890, 12),
  IR(7945, 19),
  IR(7976, 15),
  IR(8033, 22),
  IR(8161, 11),
  IR(8186, 13),
  IR(8217, 17),
  IR(8255, 20),
  IR(8294, 18),
  IR(8334, 21),
  IR(8366, 10),
  IR(8387, 10),
  IR(8420, 22),
  IR(8459, 16),
  IR(8492, 16),
  IR(8526, 10),
  IR(8757, 19),
  IR(10603, 28),
  IR(10666, 17),
  IR(11611, 27),
  IR(11655, 22),
  IR(11708, 30),
  IR(11738, 14),
  IR(11782, 29),
  IR(11828, 20),
  IR(11871, 26),
  IR(11933, 39),
  IR(12011, 42),
  IR(12092, 42),
  IR(12171, 40),
  IR(12247, 39),
  IR(12323, 40),
  IR(12399, 39),
  IR(12477, 42),
  IR(12561, 45),
  IR(12648, 45),
  IR(12711, 21),
  IR(12761, 32),
  IR(12793, 34),
  IR(12859, 31),
  IR(13063, 28),
  IR(13143, 29),
  IR(14165, 26),
  IR(14244, 32),
  IR(14314, 37),
  IR(14404, 22),
  IR(14499, 24),
  IR(14551, 27),
  IR(14593, 14),
  IR(14628, 20),
  IR(14715, 40),
  IR(14780, 24),
  IR(14839, 34),
  IR(14897, 23),
  IR(14950, 29),
  IR(14993, 13),
  IR(15026, 19),
  IR(15059, 13),
  IR(15091, 22),
  IR(15135, 25),
  IR(15188, 31),
  IR(15230, 14),
  IR(15444, 13),
  IR(15538, 18),
  IR(15605, 29),
  IR(15681, 18),
  IR(15740, 24),
  IR(15793, 28),
  IR(15887, 35),
  IR(18896, 15),
  IR(18944, 12),
  IR(18967, 14),
  IR(18997, 19),
  IR(19031, 18),
  IR(19394, 24),
  IR(19431, 19),
  IR(19561, 14),
  IR(19588, 12),
  IR(19615, 14),
  IR(19645, 15),
  IR(19683, 22),
  IR(19722, 16),
  IR(19755, 16),
  IR(19791, 19),
  IR(19831, 20),
  IR(19863, 11),
  IR(19931, 14),
  IR(19966, 20),
  IR(20003, 16),
  IR(20034, 14),
  IR(20067, 18),
  IR(20105, 19),
  IR(20140, 15),
  IR(20184, 28),
  IR(20227, 14),
  IR(20259, 17),
  IR(20302, 25),
  IR(20347, 19),
  IR(20420, 24),
  IR(20465, 20),
  IR(20506, 20),
  IR(20547, 20),
  IR(20587, 19),
  IR(20765, 15),
  IR(20863, 25),
  IR(20919, 30),
  IR(20977, 27),
  IR(21036, 31),
  IR(21101, 33),
  IR(21165, 30),
  IR(21224, 28),
  IR(21321, 34),
  IR(21385, 29),
  IR(21447, 32),
  IR(21506, 26),
  IR(21649, 14),
  IR(21692, 28),
  IR(23046, 14),
  IR(23080, 19),
  IR(23123, 23),
  IR(23171, 24),
  IR(23214, 18),
  IR(23983, 16),
  IR(24015, 15),
  IR(24040, 9),
  IR(24063, 13),
  IR(24084, 7),
  IR(24103, 11),
  IR(24290, 23),
  IR(24326, 18),
  IR(24552, 20),
  IR(24696, 20),
  IR(24744, 27),
  IR(24799, 27),
  IR(24930, 22),
  IR(24969, 20),
  IR(25005, 19),
  IR(25038, 17),
  IR(25597, 27),
  IR(25648, 23),
  IR(25990, 21),
  IR(26030, 18),
  IR(26070, 21),
  IR(26118, 26),
  IR(26159, 14),
  IR(26189, 15),
  IR(26231, 26),
  IR(26623, 24),
  IR(26666, 22),
  IR(26706, 21),
  IR(26793, 10),
  IR(16625, 5),
  IR(26976, 16),
  IR(27006, 17),
  IR(27035, 15),
  IR(27111, 28),
  IR(27426, 24),
  IR(27493, 30),
  IR(28196, 15),
  IR(28581, 15),
  IR(28620, 23),
  IR(28657, 13),
  IR(28686, 15),
  IR(28723, 21),
  IR(28766, 21),
  IR(28809, 25),
  IR(28935, 16),
  IR(28966, 14),
  IR(36221, 8),
  IR(36234, 8),
  IR(36248, 9),
  IR(36268, 14),
  IR(36293, 14),
  IR(36319, 15),
  IR(38636, 24),
  IR(38892, 27),
  IR(39974, 21),
  IR(40149, 28),
  IR(40230, 23),
  IR(42398, 21),
  IR(42440, 27),
  IR(47697, 28),
  IR(47750, 24),
  IR(47806, 31),
  IR(47867, 29),
  IR(47920, 23),
  IR(47967, 23),
  IR(48014, 23),
  IR(48061, 23),
  IR(48107, 22),
  IR(48152, 22),
  IR(48197, 22),
  IR(48242, 22),
  IR(48287, 22),
  IR(48335, 25),
  IR(48386, 25),
  IR(48436, 24),
  IR(48486, 25),
  IR(48536, 24),
  IR(49220, 15),
  IR(49252, 16),
  IR(49285, 16),
  IR(49316, 14),
  IR(49345, 14),
  IR(49377, 17),
  IR(49411, 16),
  IR(49444, 16),
  IR(49480, 19),
  IR(49514, 14),
  IR(49543, 14),
  IR(49587, 29),
  IR(49646, 29),
  IR(49698, 22),
  IR(49744, 23),
  IR(50645, 26),
  IR(50693, 24),
  IR(50742, 24),
  IR(50790, 23),
  IR(50835, 21),
  IR(50882, 25),
  IR(50930, 22),
};



static const spv::Capability kCapabilitySpans[] = {
  spv::Capability::Kernel,
  spv::Capability::Addresses,
  spv::Capability::PhysicalStorageBufferAddresses,
  spv::Capability::Shader,
  spv::Capability::ClipDistance,
  spv::Capability::CullDistance,
  spv::Capability::Geometry,
  spv::Capability::Tessellation,
  spv::Capability::RayTracingNV,
  spv::Capability::RayTracingKHR,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::Geometry,
  spv::Capability::Tessellation,
  spv::Capability::Geometry,
  spv::Capability::ShaderLayer,
  spv::Capability::ShaderViewportIndexLayerEXT,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::MultiViewport,
  spv::Capability::ShaderViewportIndex,
  spv::Capability::ShaderViewportIndexLayerEXT,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::Tessellation,
  spv::Capability::SampleRateShading,
  spv::Capability::Kernel,
  spv::Capability::GroupNonUniform,
  spv::Capability::SubgroupBallotKHR,
  spv::Capability::Kernel,
  spv::Capability::GroupNonUniform,
  spv::Capability::CoreBuiltinsARM,
  spv::Capability::SubgroupBallotKHR,
  spv::Capability::GroupNonUniformBallot,
  spv::Capability::DrawParameters,
  spv::Capability::DrawParameters,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::FragmentShadingRateKHR,
  spv::Capability::DeviceGroup,
  spv::Capability::MultiView,
  spv::Capability::TileShadingQCOM,
  spv::Capability::StencilExportEXT,
  spv::Capability::ShaderEnqueueAMDX,
  spv::Capability::DescriptorHeapEXT,
  spv::Capability::ShaderViewportMaskNV,
  spv::Capability::MeshShadingNV,
  spv::Capability::ShaderStereoViewNV,
  spv::Capability::PerViewAttributesNV,
  spv::Capability::MeshShadingNV,
  spv::Capability::FragmentFullyCoveredEXT,
  spv::Capability::MeshShadingNV,
  spv::Capability::FragmentBarycentricKHR,
  spv::Capability::FragmentDensityEXT,
  spv::Capability::MeshShadingEXT,
  spv::Capability::RayTracingNV,
  spv::Capability::RayTracingKHR,
  spv::Capability::RayTracingNV,
  spv::Capability::RayTracingMotionBlurNV,
  spv::Capability::RayTracingPositionFetchKHR,
  spv::Capability::RayTracingDisplacementMicromapNV,
  spv::Capability::RayTracingKHR,
  spv::Capability::RayTracingSpheresGeometryNV,
  spv::Capability::RayTracingLinearSweptSpheresGeometryNV,
  spv::Capability::ShaderSMBuiltinsNV,
  spv::Capability::RayTracingClusterAccelerationStructureNV,
  spv::Capability::RayCullMaskKHR,
  spv::Capability::Matrix,
  spv::Capability::Int64,
  spv::Capability::ImageBasic,
  spv::Capability::Geometry,
  spv::Capability::SampledCubeArray,
  spv::Capability::SampledRect,
  spv::Capability::Sampled1D,
  spv::Capability::SampledBuffer,
  spv::Capability::DeviceEnqueue,
  spv::Capability::Pipes,
  spv::Capability::GroupNonUniform,
  spv::Capability::Float8EXT,
  spv::Capability::CooperativeMatrixKHR,
  spv::Capability::WorkgroupMemoryExplicitLayoutKHR,
  spv::Capability::StorageBuffer16BitAccess,
  spv::Capability::VariablePointersStorageBuffer,
  spv::Capability::AtomicStorage,
  spv::Capability::StorageBuffer8BitAccess,
  spv::Capability::RayQueryKHR,
  spv::Capability::RayTracingKHR,
  spv::Capability::CooperativeMatrixKHR,
  spv::Capability::Int4TypeINTEL,
  spv::Capability::CooperativeMatrixKHR,
  spv::Capability::BFloat16TypeKHR,
  spv::Capability::BFloat16TypeKHR,
  spv::Capability::CooperativeMatrixKHR,
  spv::Capability::UntypedPointersKHR,
  spv::Capability::Linkage,
  spv::Capability::MultiViewport,
  spv::Capability::ShaderViewportIndexLayerEXT,
  spv::Capability::ShaderViewportMaskNV,
  spv::Capability::InputAttachment,
  spv::Capability::ImageBuffer,
  spv::Capability::ShaderNonUniform,
  spv::Capability::InputAttachment,
  spv::Capability::ShaderNonUniform,
  spv::Capability::SampledBuffer,
  spv::Capability::ShaderNonUniform,
  spv::Capability::ImageBuffer,
  spv::Capability::ShaderNonUniform,
  spv::Capability::CooperativeMatrixBlockLoadsNV,
  spv::Capability::VectorAnyINTEL,
  spv::Capability::Int8,
  spv::Capability::FPGAClusterAttributesALTERA,
  spv::Capability::FPGAKernelAttributesINTEL,
  spv::Capability::Subgroup2DBlockIOINTEL,
  spv::Capability::VariableLengthArrayINTEL,
  spv::Capability::UntypedPointersKHR,
  spv::Capability::SpecConditionalINTEL,
  spv::Capability::Float16,
  spv::Capability::Float8EXT,
  spv::Capability::CooperativeMatrixLayoutsARM,
  spv::Capability::Shader,
  spv::Capability::Kernel,
  spv::Capability::Shader,
  spv::Capability::UniformDecoration,
  spv::Capability::GeometryStreams,
  spv::Capability::TransformFeedback,
  spv::Capability::Kernel,
  spv::Capability::FloatControls2,
  spv::Capability::ConstantDataKHR,
  spv::Capability::SampleMaskOverrideCoverageNV,
  spv::Capability::GeometryShaderPassthroughNV,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::PushConstantBanksNV,
  spv::Capability::ShaderInvocationReorderNV,
  spv::Capability::ShaderInvocationReorderEXT,
  spv::Capability::BindlessTextureNV,
  spv::Capability::VectorComputeINTEL,
  spv::Capability::IndirectReferencesINTEL,
  spv::Capability::AsmINTEL,
  spv::Capability::FunctionFloatControlINTEL,
  spv::Capability::FPGAMemoryAttributesALTERA,
  spv::Capability::FPGAMemoryAccessesALTERA,
  spv::Capability::LoopFuseALTERA,
  spv::Capability::FPGADSPControlALTERA,
  spv::Capability::MemoryAccessAliasingINTEL,
  spv::Capability::FPGAInvocationPipeliningAttributesALTERA,
  spv::Capability::FPGABufferLocationALTERA,
  spv::Capability::IOPipesALTERA,
  spv::Capability::FPGAClusterAttributesV2ALTERA,
  spv::Capability::FPMaxErrorINTEL,
  spv::Capability::FPGALatencyControlALTERA,
  spv::Capability::FPGAArgumentInterfacesALTERA,
  spv::Capability::GlobalVariableHostAccessINTEL,
  spv::Capability::GlobalVariableFPGADecorationsALTERA,
  spv::Capability::CacheControlsINTEL,
  spv::Capability::IntrinsicSAMSUNG,
  spv::Capability::TileImageColorReadAccessEXT,
  spv::Capability::Geometry,
  spv::Capability::Tessellation,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::Geometry,
  spv::Capability::MeshShadingNV,
  spv::Capability::MeshShadingEXT,
  spv::Capability::SubgroupDispatch,
  spv::Capability::TileImageDepthReadAccessEXT,
  spv::Capability::TileImageStencilReadAccessEXT,
  spv::Capability::SampleMaskPostDepthCoverage,
  spv::Capability::DenormPreserve,
  spv::Capability::DenormFlushToZero,
  spv::Capability::SignedZeroInfNanPreserve,
  spv::Capability::RoundingModeRTE,
  spv::Capability::RoundingModeRTZ,
  spv::Capability::QuadControlKHR,
  spv::Capability::PoisonFreezeKHR,
  spv::Capability::ComputeDerivativeGroupQuadsKHR,
  spv::Capability::ComputeDerivativeGroupLinearKHR,
  spv::Capability::FragmentShaderPixelInterlockEXT,
  spv::Capability::FragmentShaderSampleInterlockEXT,
  spv::Capability::FragmentShaderShadingRateInterlockEXT,
  spv::Capability::Shader64BitIndexingEXT,
  spv::Capability::RoundToInfinityINTEL,
  spv::Capability::KernelAttributesINTEL,
  spv::Capability::FloatControls2,
  spv::Capability::RayTracingOpacityMicromapExecutionModeKHR,
  spv::Capability::FPGAKernelAttributesv2INTEL,
  spv::Capability::RegisterLimitsINTEL,
  spv::Capability::Float6EXT,
  spv::Capability::Float4EXT,
  spv::Capability::Float8UnsignedE8M0EXT,
  spv::Capability::MXInt8EXT,
  spv::Capability::FloatControls2,
  spv::Capability::FPFastMathModeINTEL,
  spv::Capability::OptNoneEXT,
  spv::Capability::RuntimeAlignedAttributeALTERA,
  spv::Capability::Kernel,
  spv::Capability::GroupNonUniformArithmetic,
  spv::Capability::GroupNonUniformBallot,
  spv::Capability::GroupNonUniformClustered,
  spv::Capability::GroupNonUniformPartitionedEXT,
  spv::Capability::ImageGatherExtended,
  spv::Capability::MinLod,
  spv::Capability::VulkanMemoryModel,
  spv::Capability::WeakLinkageAMD,
  spv::Capability::FPGALoopControlsALTERA,
  spv::Capability::ArbitraryPrecisionFixedPointALTERA,
  spv::Capability::RawAccessChainsNV,
  spv::Capability::RayTraversalPrimitiveCullingKHR,
  spv::Capability::RayTracingOpacityMicromapKHR,
  spv::Capability::RayQueryKHR,
  spv::Capability::StorageImageExtendedFormats,
  spv::Capability::Int64ImageEXT,
  spv::Capability::Shader,
  spv::Capability::VectorComputeINTEL,
  spv::Capability::GenericPointer,
  spv::Capability::FunctionPointersINTEL,
  spv::Capability::USMStorageClassesALTERA,
  spv::Capability::CooperativeMatrixTensorAddressingNV,
  spv::Capability::CooperativeMatrixDecodeVectorNV,
  spv::Capability::TensorsARM,
  spv::Capability::Addresses,
  spv::Capability::PhysicalStorageBufferAddresses,
  spv::Capability::LiteralSampler,
  spv::Capability::Addresses,
  spv::Capability::UntypedPointersKHR,
  spv::Capability::Addresses,
  spv::Capability::VariablePointers,
  spv::Capability::VariablePointersStorageBuffer,
  spv::Capability::PhysicalStorageBufferAddresses,
  spv::Capability::Kernel,
  spv::Capability::ImageQuery,
  spv::Capability::ImageQuery,
  spv::Capability::Shader,
  spv::Capability::BitInstructions,
  spv::Capability::DerivativeControl,
  spv::Capability::Groups,
  spv::Capability::SparseResidency,
  spv::Capability::PipeStorage,
  spv::Capability::NamedBarrier,
  spv::Capability::GroupNonUniformVote,
  spv::Capability::GroupNonUniformBallot,
  spv::Capability::GroupNonUniformShuffle,
  spv::Capability::GroupNonUniformShuffleRelative,
  spv::Capability::GroupNonUniformArithmetic,
  spv::Capability::GroupNonUniformClustered,
  spv::Capability::GroupNonUniformPartitionedEXT,
  spv::Capability::GroupNonUniformQuad,
  spv::Capability::Addresses,
  spv::Capability::VariablePointers,
  spv::Capability::VariablePointersStorageBuffer,
  spv::Capability::GraphARM,
  spv::Capability::BitcastExtractEXT,
  spv::Capability::SubgroupBallotKHR,
  spv::Capability::FMAKHR,
  spv::Capability::SubgroupVoteKHR,
  spv::Capability::GroupNonUniformRotateKHR,
  spv::Capability::RayTracingKHR,
  spv::Capability::RayQueryKHR,
  spv::Capability::DotProduct,
  spv::Capability::ReplicatedCompositesEXT,
  spv::Capability::TextureSampleWeightedQCOM,
  spv::Capability::TextureBoxFilterQCOM,
  spv::Capability::TextureBlockMatchQCOM,
  spv::Capability::CooperativeMatrixConversionQCOM,
  spv::Capability::TextureBlockMatch2QCOM,
  spv::Capability::ImageGatherLinearQCOM,
  spv::Capability::ImageGatherExtendedModesQCOM,
  spv::Capability::FragmentMaskAMD,
  spv::Capability::ShaderClockKHR,
  spv::Capability::AbortKHR,
  spv::Capability::ShaderInvocationReorderNV,
  spv::Capability::RayTracingMotionBlurNV,
  spv::Capability::ImageFootprintNV,
  spv::Capability::CooperativeVectorNV,
  spv::Capability::LongVectorEXT,
  spv::Capability::CooperativeVectorNV,
  spv::Capability::CooperativeVectorTrainingNV,
  spv::Capability::CooperativeMatrixConversionsNV,
  spv::Capability::DisplacementMicromapNV,
  spv::Capability::ShaderInvocationReorderEXT,
  spv::Capability::RayTracingMotionBlurNV,
  spv::Capability::RayQueryPositionFetchKHR,
  spv::Capability::RayTracingNV,
  spv::Capability::RayTracingKHR,
  spv::Capability::RayQueryKHR,
  spv::Capability::DisplacementMicromapNV,
  spv::Capability::CooperativeMatrixNV,
  spv::Capability::FragmentShaderSampleInterlockEXT,
  spv::Capability::FragmentShaderPixelInterlockEXT,
  spv::Capability::FragmentShaderShadingRateInterlockEXT,
  spv::Capability::CooperativeMatrixReductionsNV,
  spv::Capability::CooperativeMatrixPerElementOperationsNV,
  spv::Capability::TensorAddressingNV,
  spv::Capability::DemoteToHelperInvocation,
  spv::Capability::SubgroupShuffleINTEL,
  spv::Capability::SubgroupBufferBlockIOINTEL,
  spv::Capability::SubgroupImageBlockIOINTEL,
  spv::Capability::SubgroupImageMediaBlockIOINTEL,
  spv::Capability::IntegerFunctions2INTEL,
  spv::Capability::AtomicFloat16MinMaxEXT,
  spv::Capability::AtomicFloat32MinMaxEXT,
  spv::Capability::AtomicFloat64MinMaxEXT,
  spv::Capability::AtomicFloat16VectorNV,
  spv::Capability::ExpectAssumeKHR,
  spv::Capability::SubgroupAvcMotionEstimationINTEL,
  spv::Capability::SubgroupAvcMotionEstimationINTEL,
  spv::Capability::SubgroupAvcMotionEstimationIntraINTEL,
  spv::Capability::SubgroupAvcMotionEstimationINTEL,
  spv::Capability::SubgroupAvcMotionEstimationChromaINTEL,
  spv::Capability::VariableLengthArrayINTEL,
  spv::Capability::ArbitraryPrecisionFloatingPointALTERA,
  spv::Capability::UnstructuredLoopControlsINTEL,
  spv::Capability::BlockingPipesALTERA,
  spv::Capability::FPGARegALTERA,
  spv::Capability::AtomicFloat16AddEXT,
  spv::Capability::AtomicFloat32AddEXT,
  spv::Capability::AtomicFloat64AddEXT,
  spv::Capability::AtomicFloat16VectorNV,
  spv::Capability::LongCompositesINTEL,
  spv::Capability::BFloat16ConversionINTEL,
  spv::Capability::SplitBarrierEXT,
  spv::Capability::ArithmeticFenceEXT,
  spv::Capability::TaskSequenceALTERA,
  spv::Capability::SubgroupBufferPrefetchINTEL,
  spv::Capability::Subgroup2DBlockTransformINTEL,
  spv::Capability::Subgroup2DBlockTransposeINTEL,
  spv::Capability::SubgroupMatrixMultiplyAccumulateINTEL,
  spv::Capability::TernaryBitwiseFunctionINTEL,
  spv::Capability::UntypedVariableLengthArrayINTEL,
  spv::Capability::FunctionVariantsINTEL,
  spv::Capability::PredicatedIOINTEL,
  spv::Capability::GroupUniformArithmeticKHR,
  spv::Capability::TensorFloat32RoundingINTEL,
  spv::Capability::MaskedGatherScatterINTEL,
  spv::Capability::BindlessImagesINTEL,
  spv::Capability::DotProductFloat16AccFloat32VALVE,
  spv::Capability::DotProductBFloat16AccVALVE,
  spv::Capability::DotProductFloat16AccFloat16VALVE,
  spv::Capability::DotProductBFloat16AccVALVE,
  spv::Capability::DotProductFloat8AccFloat32VALVE,
  spv::Capability::Float64,
  spv::Capability::InterpolationFunction,
};



static const spvtools::Extension kExtensionSpans[] = {
  spvtools::Extension::kSPV_EXT_physical_storage_buffer,
  spvtools::Extension::kSPV_KHR_physical_storage_buffer,
  spvtools::Extension::kSPV_KHR_shader_ballot,
  spvtools::Extension::kSPV_KHR_shader_draw_parameters,
  spvtools::Extension::kSPV_KHR_shader_draw_parameters,
  spvtools::Extension::kSPV_NV_mesh_shader,
  spvtools::Extension::kSPV_EXT_mesh_shader,
  spvtools::Extension::kSPV_KHR_fragment_shading_rate,
  spvtools::Extension::kSPV_KHR_device_group,
  spvtools::Extension::kSPV_KHR_multiview,
  spvtools::Extension::kSPV_AMD_shader_explicit_vertex_parameter,
  spvtools::Extension::kSPV_EXT_shader_stencil_export,
  spvtools::Extension::kSPV_NV_viewport_array2,
  spvtools::Extension::kSPV_NV_mesh_shader,
  spvtools::Extension::kSPV_NV_stereo_view_rendering,
  spvtools::Extension::kSPV_NVX_multiview_per_view_attributes,
  spvtools::Extension::kSPV_NV_mesh_shader,
  spvtools::Extension::kSPV_EXT_fragment_fully_covered,
  spvtools::Extension::kSPV_NV_mesh_shader,
  spvtools::Extension::kSPV_NV_fragment_shader_barycentric,
  spvtools::Extension::kSPV_KHR_fragment_shader_barycentric,
  spvtools::Extension::kSPV_EXT_fragment_invocation_density,
  spvtools::Extension::kSPV_NV_shading_rate,
  spvtools::Extension::kSPV_EXT_mesh_shader,
  spvtools::Extension::kSPV_NV_ray_tracing,
  spvtools::Extension::kSPV_KHR_ray_tracing,
  spvtools::Extension::kSPV_NV_ray_tracing,
  spvtools::Extension::kSPV_NV_ray_tracing_motion_blur,
  spvtools::Extension::kSPV_KHR_ray_tracing,
  spvtools::Extension::kSPV_NV_linear_swept_spheres,
  spvtools::Extension::kSPV_NV_shader_sm_builtins,
  spvtools::Extension::kSPV_NV_cluster_acceleration_structure,
  spvtools::Extension::kSPV_KHR_ray_cull_mask,
  spvtools::Extension::kSPV_AMD_shader_ballot,
  spvtools::Extension::kSPV_ARM_core_builtins,
  spvtools::Extension::kSPV_EXT_shader_tile_image,
  spvtools::Extension::kSPV_ARM_tensors,
  spvtools::Extension::kSPV_ARM_graph,
  spvtools::Extension::kSPV_ARM_cooperative_matrix_layouts,
  spvtools::Extension::kSPV_EXT_float8,
  spvtools::Extension::kSPV_EXT_ocp_microscaling_types,
  spvtools::Extension::kSPV_KHR_workgroup_memory_explicit_layout,
  spvtools::Extension::kSPV_KHR_subgroup_vote,
  spvtools::Extension::kSPV_KHR_16bit_storage,
  spvtools::Extension::kSPV_KHR_variable_pointers,
  spvtools::Extension::kSPV_KHR_shader_atomic_counter_ops,
  spvtools::Extension::kSPV_KHR_post_depth_coverage,
  spvtools::Extension::kSPV_KHR_8bit_storage,
  spvtools::Extension::kSPV_KHR_float_controls,
  spvtools::Extension::kSPV_KHR_ray_query,
  spvtools::Extension::kSPV_KHR_untyped_pointers,
  spvtools::Extension::kSPV_KHR_ray_query,
  spvtools::Extension::kSPV_KHR_ray_tracing,
  spvtools::Extension::kSPV_QCOM_image_processing,
  spvtools::Extension::kSPV_QCOM_tile_shading,
  spvtools::Extension::kSPV_QCOM_cooperative_matrix_conversion,
  spvtools::Extension::kSPV_QCOM_image_processing2,
  spvtools::Extension::kSPV_QCOM_multiple_wait_queues,
  spvtools::Extension::kSPV_QCOM_image_processing3,
  spvtools::Extension::kSPV_AMD_gpu_shader_half_float_fetch,
  spvtools::Extension::kSPV_AMD_texture_gather_bias_lod,
  spvtools::Extension::kSPV_AMD_shader_fragment_mask,
  spvtools::Extension::kSPV_AMD_shader_image_load_store_lod,
  spvtools::Extension::kSPV_EXT_shader_image_int64,
  spvtools::Extension::kSPV_KHR_shader_clock,
  spvtools::Extension::kSPV_AMDX_shader_enqueue,
  spvtools::Extension::kSPV_KHR_quad_control,
  spvtools::Extension::kSPV_INTEL_int4,
  spvtools::Extension::kSPV_KHR_bfloat16,
  spvtools::Extension::kSPV_KHR_abort,
  spvtools::Extension::kSPV_EXT_descriptor_heap,
  spvtools::Extension::kSPV_KHR_constant_data,
  spvtools::Extension::kSPV_KHR_poison_freeze,
  spvtools::Extension::kSPV_AMD_weak_linkage,
  spvtools::Extension::kSPV_NV_sample_mask_override_coverage,
  spvtools::Extension::kSPV_NV_geometry_shader_passthrough,
  spvtools::Extension::kSPV_EXT_shader_viewport_index_layer,
  spvtools::Extension::kSPV_NV_viewport_array2,
  spvtools::Extension::kSPV_NV_viewport_array2,
  spvtools::Extension::kSPV_NVX_multiview_per_view_attributes,
  spvtools::Extension::kSPV_NV_shader_image_footprint,
  spvtools::Extension::kSPV_NV_compute_shader_derivatives,
  spvtools::Extension::kSPV_KHR_compute_shader_derivatives,
  spvtools::Extension::kSPV_NV_shader_subgroup_partitioned,
  spvtools::Extension::kSPV_EXT_shader_subgroup_partitioned,
  spvtools::Extension::kSPV_EXT_descriptor_indexing,
  spvtools::Extension::kSPV_KHR_ray_tracing_position_fetch,
  spvtools::Extension::kSPV_KHR_vulkan_memory_model,
  spvtools::Extension::kSPV_NV_cooperative_matrix,
  spvtools::Extension::kSPV_EXT_fragment_shader_interlock,
  spvtools::Extension::kSPV_EXT_demote_to_helper_invocation,
  spvtools::Extension::kSPV_NV_displacement_micromap,
  spvtools::Extension::kSPV_KHR_opacity_micromap,
  spvtools::Extension::kSPV_EXT_opacity_micromap,
  spvtools::Extension::kSPV_NV_shader_invocation_reorder,
  spvtools::Extension::kSPV_EXT_shader_invocation_reorder,
  spvtools::Extension::kSPV_NV_bindless_texture,
  spvtools::Extension::kSPV_NV_cooperative_vector,
  spvtools::Extension::kSPV_NV_shader_atomic_fp16_vector,
  spvtools::Extension::kSPV_NV_raw_access_chains,
  spvtools::Extension::kSPV_NV_push_constant_bank,
  spvtools::Extension::kSPV_EXT_long_vector,
  spvtools::Extension::kSPV_EXT_shader_64bit_indexing,
  spvtools::Extension::kSPV_NV_cooperative_matrix2,
  spvtools::Extension::kSPV_NV_tensor_addressing,
  spvtools::Extension::kSPV_NV_cooperative_matrix_decode_vector,
  spvtools::Extension::kSPV_INTEL_subgroups,
  spvtools::Extension::kSPV_INTEL_media_block_io,
  spvtools::Extension::kSPV_INTEL_float_controls2,
  spvtools::Extension::kSPV_INTEL_shader_integer_functions2,
  spvtools::Extension::kSPV_INTEL_function_pointers,
  spvtools::Extension::kSPV_INTEL_inline_assembly,
  spvtools::Extension::kSPV_EXT_shader_atomic_float_min_max,
  spvtools::Extension::kSPV_INTEL_vector_compute,
  spvtools::Extension::kSPV_KHR_expect_assume,
  spvtools::Extension::kSPV_INTEL_device_side_avc_motion_estimation,
  spvtools::Extension::kSPV_INTEL_variable_length_array,
  spvtools::Extension::kSPV_ALTERA_fpga_memory_attributes,
  spvtools::Extension::kSPV_INTEL_fpga_memory_attributes,
  spvtools::Extension::kSPV_INTEL_fp_fast_math_mode,
  spvtools::Extension::kSPV_ALTERA_arbitrary_precision_integers,
  spvtools::Extension::kSPV_INTEL_arbitrary_precision_integers,
  spvtools::Extension::kSPV_ALTERA_arbitrary_precision_floating_point,
  spvtools::Extension::kSPV_INTEL_arbitrary_precision_floating_point,
  spvtools::Extension::kSPV_INTEL_unstructured_loop_controls,
  spvtools::Extension::kSPV_ALTERA_fpga_loop_controls,
  spvtools::Extension::kSPV_INTEL_fpga_loop_controls,
  spvtools::Extension::kSPV_INTEL_kernel_attributes,
  spvtools::Extension::kSPV_ALTERA_fpga_memory_accesses,
  spvtools::Extension::kSPV_INTEL_fpga_memory_accesses,
  spvtools::Extension::kSPV_ALTERA_fpga_cluster_attributes,
  spvtools::Extension::kSPV_INTEL_fpga_cluster_attributes,
  spvtools::Extension::kSPV_ALTERA_loop_fuse,
  spvtools::Extension::kSPV_INTEL_loop_fuse,
  spvtools::Extension::kSPV_ALTERA_fpga_dsp_control,
  spvtools::Extension::kSPV_INTEL_fpga_dsp_control,
  spvtools::Extension::kSPV_INTEL_memory_access_aliasing,
  spvtools::Extension::kSPV_ALTERA_fpga_invocation_pipelining_attributes,
  spvtools::Extension::kSPV_INTEL_fpga_invocation_pipelining_attributes,
  spvtools::Extension::kSPV_ALTERA_fpga_buffer_location,
  spvtools::Extension::kSPV_INTEL_fpga_buffer_location,
  spvtools::Extension::kSPV_ALTERA_arbitrary_precision_fixed_point,
  spvtools::Extension::kSPV_INTEL_arbitrary_precision_fixed_point,
  spvtools::Extension::kSPV_ALTERA_usm_storage_classes,
  spvtools::Extension::kSPV_INTEL_usm_storage_classes,
  spvtools::Extension::kSPV_ALTERA_runtime_aligned,
  spvtools::Extension::kSPV_INTEL_runtime_aligned,
  spvtools::Extension::kSPV_ALTERA_io_pipes,
  spvtools::Extension::kSPV_INTEL_io_pipes,
  spvtools::Extension::kSPV_ALTERA_blocking_pipes,
  spvtools::Extension::kSPV_INTEL_blocking_pipes,
  spvtools::Extension::kSPV_ALTERA_fpga_reg,
  spvtools::Extension::kSPV_INTEL_fpga_reg,
  spvtools::Extension::kSPV_KHR_integer_dot_product,
  spvtools::Extension::kSPV_KHR_cooperative_matrix,
  spvtools::Extension::kSPV_EXT_replicated_composites,
  spvtools::Extension::kSPV_KHR_bit_instructions,
  spvtools::Extension::kSPV_KHR_subgroup_rotate,
  spvtools::Extension::kSPV_KHR_float_controls2,
  spvtools::Extension::kSPV_KHR_fma,
  spvtools::Extension::kSPV_KHR_opacity_micromap,
  spvtools::Extension::kSPV_EXT_shader_atomic_float_add,
  spvtools::Extension::kSPV_INTEL_long_composites,
  spvtools::Extension::kSPV_EXT_optnone,
  spvtools::Extension::kSPV_INTEL_optnone,
  spvtools::Extension::kSPV_EXT_shader_atomic_float16_add,
  spvtools::Extension::kSPV_INTEL_debug_module,
  spvtools::Extension::kSPV_INTEL_bfloat16_conversion,
  spvtools::Extension::kSPV_EXT_split_barrier,
  spvtools::Extension::kSPV_INTEL_split_barrier,
  spvtools::Extension::kSPV_EXT_arithmetic_fence,
  spvtools::Extension::kSPV_ALTERA_task_sequence,
  spvtools::Extension::kSPV_INTEL_task_sequence,
  spvtools::Extension::kSPV_INTEL_fp_max_error,
  spvtools::Extension::kSPV_ALTERA_fpga_latency_control,
  spvtools::Extension::kSPV_INTEL_fpga_latency_control,
  spvtools::Extension::kSPV_ALTERA_fpga_argument_interfaces,
  spvtools::Extension::kSPV_INTEL_fpga_argument_interfaces,
  spvtools::Extension::kSPV_INTEL_global_variable_host_access,
  spvtools::Extension::kSPV_ALTERA_global_variable_fpga_decorations,
  spvtools::Extension::kSPV_INTEL_global_variable_fpga_decorations,
  spvtools::Extension::kSPV_INTEL_subgroup_buffer_prefetch,
  spvtools::Extension::kSPV_INTEL_2d_block_io,
  spvtools::Extension::kSPV_INTEL_subgroup_matrix_multiply_accumulate,
  spvtools::Extension::kSPV_INTEL_ternary_bitwise_function,
  spvtools::Extension::kSPV_INTEL_function_variants,
  spvtools::Extension::kSPV_INTEL_predicated_io,
  spvtools::Extension::kSPV_INTEL_rounded_divide_sqrt,
  spvtools::Extension::kSPV_KHR_uniform_group_instructions,
  spvtools::Extension::kSPV_INTEL_tensor_float32_conversion,
  spvtools::Extension::kSPV_INTEL_masked_gather_scatter,
  spvtools::Extension::kSPV_INTEL_cache_controls,
  spvtools::Extension::kSPV_INTEL_maximum_registers,
  spvtools::Extension::kSPV_INTEL_bindless_images,
  spvtools::Extension::kSPV_VALVE_mixed_float_dot_product,
  spvtools::Extension::kSPV_SAMSUNG_intrinsic,
  spvtools::Extension::kSPV_KHR_no_integer_wrap_decoration,
  spvtools::Extension::kSPV_NV_mesh_shader,
  spvtools::Extension::kSPV_EXT_mesh_shader,
  spvtools::Extension::kSPV_GOOGLE_hlsl_functionality1,
  spvtools::Extension::kSPV_GOOGLE_user_type,
  spvtools::Extension::kSPV_KHR_subgroup_uniform_control_flow,
  spvtools::Extension::kSPV_AMD_shader_early_and_late_fragment_tests,
  spvtools::Extension::kSPV_AMD_shader_early_and_late_fragment_tests,
  spvtools::Extension::kSPV_EXT_shader_stencil_export,
  spvtools::Extension::kSPV_KHR_maximal_reconvergence,
  spvtools::Extension::kSPV_KHR_linkonce_odr,
  spvtools::Extension::kSPV_KHR_storage_buffer_storage_class,
  spvtools::Extension::kSPV_KHR_variable_pointers,
  spvtools::Extension::kSPV_KHR_terminate_invocation,
  spvtools::Extension::kSPV_KHR_relaxed_extended_instruction,
  spvtools::Extension::kSPV_KHR_ray_tracing,
  spvtools::Extension::kSPV_KHR_ray_query,
  spvtools::Extension::kSPV_NV_ray_tracing,
  spvtools::Extension::kSPV_KHR_ray_tracing,
  spvtools::Extension::kSPV_KHR_ray_query,
  spvtools::Extension::kSPV_NV_displacement_micromap,
  spvtools::Extension::kSPV_GOOGLE_decorate_string,
  spvtools::Extension::kSPV_GOOGLE_hlsl_functionality1,
};



static const spv_operand_type_t kOperandSpans[] = {
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_BUILT_IN,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE,
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE,
  SPV_OPERAND_TYPE_FP_FAST_MATH_MODE,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_LINKAGE_TYPE,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_FPDENORM_MODE,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_FPOPERATION_MODE,
  SPV_OPERAND_TYPE_LITERAL_FLOAT,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ACCESS_QUALIFIER,
  SPV_OPERAND_TYPE_HOST_ACCESS_QUALIFIER,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_INITIALIZATION_MODE_QUALIFIER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LOAD_CACHE_CONTROL,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_STORE_CACHE_CONTROL,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_NAMED_MAXIMUM_NUMBER_OF_REGISTERS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SOURCE_LANGUAGE,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_STRING,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER,
  SPV_OPERAND_TYPE_ADDRESSING_MODEL,
  SPV_OPERAND_TYPE_MEMORY_MODEL,
  SPV_OPERAND_TYPE_EXECUTION_MODEL,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_EXECUTION_MODE,
  SPV_OPERAND_TYPE_CAPABILITY,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_OPTIONAL_FPENCODING,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DIMENSIONALITY,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT,
  SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ACCESS_QUALIFIER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_FUNCTION_CONTROL,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DECORATION,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_DECORATION,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_IMAGE,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_IMAGE,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LOOP_CONTROL,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SELECTION_CONTROL,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_GROUP_OPERATION,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_GROUP_OPERATION,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_TENSOR_OPERANDS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_PACKED_VECTOR_FORMAT,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_IMAGE,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_COOPERATIVE_MATRIX_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SCOPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_COOPERATIVE_MATRIX_REDUCE,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_MEMORY_ACCESS,
  SPV_OPERAND_TYPE_TENSOR_ADDRESSING_OPERANDS,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_RAW_ACCESS_CHAIN_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_MATRIX_MULTIPLY_ACCUMULATE_OPERANDS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_EXECUTION_MODEL,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_STRING,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CAPABILITY,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_VARIABLE_CAPABILITY,
  SPV_OPERAND_TYPE_TYPE_ID,
  SPV_OPERAND_TYPE_RESULT_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_TYPE_QUALIFIER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_COMPOSITE_TYPE,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_DEBUG_OPERATION,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_SOURCE_LANGUAGE,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_BASE_TYPE_ATTRIBUTE_ENCODING,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_STORAGE_CLASS,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_TYPE_QUALIFIER,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_COMPOSITE_TYPE,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_VARIABLE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_OPTIONAL_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_INFO_FLAGS,
  SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_OPERATION,
  SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_CLDEBUG100_DEBUG_IMPORTED_ENTITY,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_LITERAL_INTEGER,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_ID,
  SPV_OPERAND_TYPE_FP_ROUNDING_MODE,
};
