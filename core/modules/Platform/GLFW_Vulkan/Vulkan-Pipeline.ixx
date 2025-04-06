module;

#include <vector>
#include <string>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <vulkan/vulkan.h>

export module Vulkan:Pipeline;

import :Device;

// TODO: Break up massive configuration structure through strategy pattern.
/*		Design:
*			Each new pipeline configuration through VulkanPipelineBuilder::Configure(pipeline_type) should
*			create a new builder strategy based on the respective type. This builder should hold the necessary
*			data to build that specific pipeline.
*/

export
{
	struct VulkanPipeline
	{
		// Handles
		VkPipeline handle = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkRenderPass render_pass = VK_NULL_HANDLE;

		// Descriptor pools
		std::vector<VkDescriptorPool> descriptor_pools;

		// Pipeline resource descriptions
		std::vector<VkDescriptorSetLayout> ds_layouts;
		std::vector<VkPushConstantRange> push_constants;
	};

	struct VulkanRenderPassConfiguration
	{
		struct Subpass
		{
			VkSubpassDescription description;
			std::vector<VkAttachmentReference> input_attachments;
			std::vector<VkAttachmentReference> color_attachments;
			std::vector<VkAttachmentReference> resolve_attachments;
			std::vector<uint32_t> preserve_attachments;
		};

		VkRenderPassCreateInfo create_info{};
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkSubpassDescription> subpass_descriptions;
		std::vector<VkSubpassDependency> subpass_dependencies;

		std::vector<Subpass> subpasses;
	};

	struct VulkanPipelineConfiguration
	{
		// Maps this configuration to a specific pipeline type
		VkStructureType type;

		// Pipeline Create Infos
		VkComputePipelineCreateInfo compute_info{};
		VkGraphicsPipelineCreateInfo graphics_info{};
		VkRayTracingPipelineCreateInfoKHR raytracing_info{};

		// Graphics Pipeline Configuration Structures
		// ------------------------------------------

		// Render Pass
		VulkanRenderPassConfiguration render_pass{};

		// Dynamic Rendering
		bool use_dynamic_rendering = false;
		VkPipelineRenderingCreateInfo dynamic_render_info{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		std::vector<VkFormat> dynamic_color_attachment_formats;

		// Vertex Input
		VkPipelineVertexInputStateCreateInfo vertex_input_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		std::vector<VkVertexInputBindingDescription> vertex_bindings;
		std::vector<VkVertexInputAttributeDescription> vertex_attributes;

		// Input Assembly and Tessellation
		VkPipelineInputAssemblyStateCreateInfo input_assembly_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		VkPipelineTessellationStateCreateInfo tessellation_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

		// Viewport state
		VkPipelineViewportStateCreateInfo viewport_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		std::vector<VkViewport> viewports;
		std::vector<VkRect2D> scissors;

		VkPipelineRasterizationStateCreateInfo rasterization_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

		VkPipelineMultisampleStateCreateInfo multisample_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		std::vector<VkSampleMask> multisample_masks;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		VkPipelineColorBlendStateCreateInfo color_blend_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;

		VkPipelineDynamicStateCreateInfo dynamic_state{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		std::vector<VkDynamicState> dynamic_states;

		// Shader information for this pipeline
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

		// Information about the desired layout
		VkPipelineLayoutCreateInfo layout_info{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

		// Descriptor Set Layouts
		std::vector<VkDescriptorSetLayout> ds_layouts;

		// Temp structures for building descriptor set layouts
		VkDescriptorSetLayoutCreateInfo ds_layout_info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		// Push Constants
		std::vector<VkPushConstantRange> push_constants;
	};

	class VulkanPipelineBuilder
	{
	public:
		struct Result
		{
			std::vector<VulkanPipeline*> compute_pipelines;
			std::vector<VulkanPipeline*> graphics_pipelines;
			std::vector<VulkanPipeline*> ray_tracing_pipelines;
		};

	public:
		VulkanPipelineBuilder();
		~VulkanPipelineBuilder();

		void Initialize(VulkanDevice* device, std::vector<VulkanPipeline>& pipeline_buffer);

		void Cleanup();

		// Start the configuration chain
		VulkanPipelineBuilder& Configure(const VkStructureType& pipeline_type, const VkPipelineCreateFlags& create_flags = 0);

		// End the configuration chain
		Result Build();

		// Shader Configuration
		// --------------------

		// Generate a shader module
		VulkanPipelineBuilder& BindShader(const VkShaderStageFlagBits& shader_stage, const VkPipelineShaderStageCreateFlags& create_flags, 
			const std::string& file_path, bool is_hlsl = false, const char* entry_point = "main");

		// Layout Configuration
		// ------------------------

		// Begins the configuration chain for a new pipeline layout.
		VulkanPipelineBuilder& ConfigurePipelineLayout(const VkPipelineLayoutCreateFlags& create_flags = 0);

		// Begins the configuration chain for a new descriptor set layout
		VulkanPipelineBuilder& ConfigureDescSetLayout(const VkDescriptorSetLayoutCreateFlags& create_flags = 0);

		// Adds a binding to the current descriptor set layout
		VulkanPipelineBuilder& AddDescSetLayoutBinding(const uint32_t& binding, const VkDescriptorType& desc_type, const uint32_t& count, 
			const VkShaderStageFlags& shader_stages, const VkSampler* immutable_sampler = nullptr);

		// Ends the configuration chain and creates a new descriptor set layout
		VulkanPipelineBuilder& BuildDescSetLayout();

		// Add a push constant to the current pipeline layout
		VulkanPipelineBuilder& AddPushConstantRange(const VkShaderStageFlags& shader_stages, const uint32_t& offset, const uint32_t& size);

		// Ends the configuration chain for a new pipeline layout
		VulkanPipelineBuilder& BuildPipelineLayout();

		// Graphics Configuration
		// ----------------------

		// Dynamic Rendering

		VulkanPipelineBuilder& UseDynamicRendering();

		VulkanPipelineBuilder& SetDynamicViewMask(const uint32_t& view_mask);

		VulkanPipelineBuilder& AddDynamicColorAttachmentFormat(const VkFormat& format);

		VulkanPipelineBuilder& SetDynamicDepthAttachmentFormat(const VkFormat& format);

		VulkanPipelineBuilder& SetDynamicStencilAttachmentFormat(const VkFormat& format);

		// Render Pass Configuration
		VulkanPipelineBuilder& ConfigureRenderPass(const VkRenderPassCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& AddRenderPassAtachment(const VkAttachmentDescription& description);

		VulkanPipelineBuilder& ConfigureNewSubPass(const VkSubpassDescriptionFlags& create_flags, const VkPipelineBindPoint& bind_point);

		VulkanPipelineBuilder& AddSubpassInputAttachment(const uint32_t& attachment, const VkImageLayout& layout);

		VulkanPipelineBuilder& AddSubpassColorAttachment(const uint32_t& attachment, const VkImageLayout& layout);

		VulkanPipelineBuilder& AddSubpassResolveAttachment(const uint32_t& attachment, const VkImageLayout& layout);

		VulkanPipelineBuilder& AddSubpassPreserveAttachment(const uint32_t& attachment);

		VulkanPipelineBuilder& BuildSubpass();

		VulkanPipelineBuilder& AddSubpassDependency(const VkSubpassDependency& dependency);

		VulkanPipelineBuilder& BuildRenderPass();

		// Vertex Input State
		VulkanPipelineBuilder& ConfigureVertexInputState(const VkPipelineVertexInputStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& AddVertexBindingDescription(const uint32_t& binding, const uint32_t& stride, const VkVertexInputRate& input_rate);

		VulkanPipelineBuilder& AddVertexAttributeDescription(const uint32_t& location, const uint32_t& binding, const VkFormat& format, const uint32_t& offset);

		VulkanPipelineBuilder& BuildVertexInputState();

		// Input Assembly State
		VulkanPipelineBuilder& ConfigureInputAssemblyState(const VkPipelineInputAssemblyStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& SetPrimitiveTopology(const VkPrimitiveTopology& topology);

		VulkanPipelineBuilder& SetPrimitiveRestartEnable(const VkBool32& prim_restart_enable);

		// Tessellation State
		VulkanPipelineBuilder& ConfigureTessellationState(const VkPipelineTessellationStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& SetPatchControlPointCount(const uint32_t& count);

		// Viewport State
		VulkanPipelineBuilder& ConfigureViewportState(const VkPipelineViewportStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& AddViewport(const VkViewport& viewport);

		VulkanPipelineBuilder& AddScissor(const VkRect2D& scissor);

		VulkanPipelineBuilder& BuildViewportState();

		// Rasterization State
		VulkanPipelineBuilder& ConfigureRasterizationState(const VkPipelineRasterizationStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& SetRasterizerDiscardEnabled(const VkBool32& rast_discard_enable);

		VulkanPipelineBuilder& SetPolygonMode(const VkPolygonMode& mode);

		VulkanPipelineBuilder& SetCullMode(const VkCullModeFlags& mode);

		VulkanPipelineBuilder& SetFrontFace(const VkFrontFace& front_face);

		VulkanPipelineBuilder& SetDepthClampEnabled(const VkBool32& depth_clamp_enable);

		VulkanPipelineBuilder& SetDepthBiasEnabled(const VkBool32& depth_bias_enable);

		VulkanPipelineBuilder& SetDepthBiasConstantFactor(const float& constant_factor);

		VulkanPipelineBuilder& SetDepthBiasClamp(const float& bias_clamp);

		VulkanPipelineBuilder& SetDepthBiasSlopeFactor(const float& slope_factor);

		VulkanPipelineBuilder& SetLineWidth(const float& line_width);

		// Multisample State
		VulkanPipelineBuilder& ConfigureMultisampleState(const VkPipelineMultisampleStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& SetRasterizationSamples(const VkSampleCountFlagBits& sample_count);

		VulkanPipelineBuilder& SetSampleShadingEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetMinSampleShading(const float& min_sample_shading);

		VulkanPipelineBuilder& AddSampleMask(const VkSampleMask& mask);

		VulkanPipelineBuilder& SetAlphaToCoverageEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetAlphaToOneEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& BuildMultisampleState();

		// Depth Stencil State
		VulkanPipelineBuilder& ConfigureDepthStencilState(const VkPipelineDepthStencilStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& SetDepthTestEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetDepthWriteEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetDepthCompareOp(const VkCompareOp& op);

		VulkanPipelineBuilder& SetDepthBoundsTestEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetStencilTestEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetFrontStencilOpState(const VkStencilOpState& state);

		VulkanPipelineBuilder& SetBackStencilOpState(const VkStencilOpState& state);

		VulkanPipelineBuilder& SetMinDepthBounds(const float& min_bounds);

		VulkanPipelineBuilder& SetMaxDepthBounds(const float& max_bounds);

		// Color Blend State
		VulkanPipelineBuilder& ConfigureColorBlendState(const VkPipelineColorBlendStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& SetLogicOpEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetLogicOp(const VkLogicOp& op);

		VulkanPipelineBuilder& SetBlendConstants(const float& r, const float& g, const float& b, const float& a);

		VulkanPipelineBuilder& AddColorAttachment();

		VulkanPipelineBuilder& SetBlendEnabled(const VkBool32& enable);

		VulkanPipelineBuilder& SetSrcColorBlendFactor(const VkBlendFactor& blend_factor);

		VulkanPipelineBuilder& SetDstColorBlendFactor(const VkBlendFactor& blend_factor);

		VulkanPipelineBuilder& SetColorBlendOp(const VkBlendOp& blend_op);

		VulkanPipelineBuilder& SetSrcAlphaBlendFactor(const VkBlendFactor& blend_factor);

		VulkanPipelineBuilder& SetDstAlphaBlendFactor(const VkBlendFactor& blend_factor);

		VulkanPipelineBuilder& SetAlphaBlendOp(const VkBlendOp& blend_op);

		VulkanPipelineBuilder& SetColorWriteMask(const VkColorComponentFlags& write_mask);

		VulkanPipelineBuilder& BuildColorBlendState();

		// Dynamic State
		VulkanPipelineBuilder& ConfigureDynamicState(const VkPipelineDynamicStateCreateFlags& create_flags = 0);

		VulkanPipelineBuilder& AddDynamicState(const VkDynamicState& state);

		VulkanPipelineBuilder& BuildDynamicState();

		// Raytracing Configuration
		// ------------------------

	private:
		shaderc_shader_kind VkShaderToShaderc(const VkShaderStageFlags& flags);

	private:
		// Renderer References
		VulkanDevice* m_logical_device;
		std::vector<VulkanPipeline>* m_pipeline_buffer;

		// Configurations to build
		std::vector<VulkanPipelineConfiguration> m_configurations;

		// Build Result
		Result m_build_result;
	};
}