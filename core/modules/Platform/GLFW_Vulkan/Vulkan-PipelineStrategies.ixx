module;

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

export module Vulkan:PipelineStrategies;

import :Pipeline;

export
{
	class VulkanGraphicsPipeline : public IVulkanPipelineStrategy
	{
	public:
		struct DynamicRenderingInfo
		{
			bool enabled = false;
			VkPipelineRenderingCreateInfo create_info{};
			std::vector<VkFormat> color_attachment_formats;
		};

		struct RenderPass
		{
			struct Subpass
			{
				VkSubpassDescription description{};
				std::vector<VkAttachmentReference> input_attachments;
				std::vector<VkAttachmentReference> color_attachments;
				std::vector<VkAttachmentReference> resolve_attachments;
				std::vector<uint32_t> preserve_attachments;
			};

			bool enabled = false;

			VkRenderPassCreateInfo create_info{};
			std::vector<VkAttachmentDescription> attachments;
			std::vector<VkSubpassDescription> subpass_descriptions;
			std::vector<VkSubpassDependency> subpass_dependencies;

			std::vector<Subpass> subpasses;
		};

		struct ConfigurationState
		{
			VkPipelineVertexInputStateCreateInfo vertex_input{};
			VkPipelineInputAssemblyStateCreateInfo input_assembly{};
			VkPipelineTessellationStateCreateInfo tessellation{};
			VkPipelineViewportStateCreateInfo viewport{};
			VkPipelineRasterizationStateCreateInfo rasterization{};
			VkPipelineMultisampleStateCreateInfo multisampling{};
			VkPipelineDepthStencilStateCreateInfo depth_stencil{};
			VkPipelineColorBlendStateCreateInfo color_blend{};
			VkPipelineDynamicStateCreateInfo dynamic_state{};

			DynamicRenderingInfo dynamic_rendering;
			RenderPass render_pass;
		};

	protected:
		VulkanPipelineInfo& Build() override;

	public:
		VulkanGraphicsPipeline();
		virtual ~VulkanGraphicsPipeline() override;

		virtual VulkanPipelineType GetType() override;
		void Initialize(VulkanDevice* device) override;

		virtual VulkanGraphicsPipeline& BindShader(const VkPipelineShaderStageCreateInfo& stage_info) override;
		virtual VulkanGraphicsPipeline& ConfigurePipelineLayout(const VkPipelineLayoutCreateFlags& create_flags = 0) override;
		virtual VulkanGraphicsPipeline& AddDescriptorSetLayout(const VkDescriptorSetLayout& layout) override;
		virtual VulkanGraphicsPipeline& AddPushConstantRange(const VkPushConstantRange& range) override;

		// Graphics Pipeline Specific Details

		// -------------------------------
		// Dynamic Rendering
		// -------------------------------

		VulkanGraphicsPipeline& SetDynamicViewMask(const uint32_t& view_mask);
		VulkanGraphicsPipeline& AddDynamicColorAttachmentFormat(const VkFormat& format);
		VulkanGraphicsPipeline& SetDynamicDepthAttachmentFormat(const VkFormat& format);
		VulkanGraphicsPipeline& SetDynamicStencilAttachmentFormat(const VkFormat& format);
		VulkanGraphicsPipeline& SetDynamicRenderingEXT(void* extension);

		// -------------------------------
		// Render Pass
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureRenderPass(const VkRenderPassCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& AddRenderPassAttachment(const VkAttachmentDescription& description);
		VulkanGraphicsPipeline& AddSubpassDependency(const VkSubpassDependency& dependency);

		// Render Pass - Sub Pass

		VulkanGraphicsPipeline& BeginSubpass(const VkSubpassDescriptionFlags& create_flags, const VkPipelineBindPoint& bind_point);
		VulkanGraphicsPipeline& AddSubpassInputAttachment(const uint32_t& attachment, const VkImageLayout& layout);
		VulkanGraphicsPipeline& AddSubpassColorAttachment(const uint32_t& attachment, const VkImageLayout& layout);
		VulkanGraphicsPipeline& AddSubpassResolveAttachment(const uint32_t& attachment, const VkImageLayout& layout);
		VulkanGraphicsPipeline& AddSubpassPreserveAttachment(const uint32_t& attachment);
		VulkanGraphicsPipeline& EndSubpass();

		// -------------------------------
		// Vertex Input State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureVertexInputState(const VkPipelineVertexInputStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& AddVertexBindingDescription(const uint32_t& binding, const uint32_t& stride, const VkVertexInputRate& input_rate);
		VulkanGraphicsPipeline& AddVertexAttributeDescription(const uint32_t& location, const uint32_t& binding, const VkFormat& format, const uint32_t& offset);

		// -------------------------------
		// Input Assembly State
		// -------------------------------
		
		VulkanGraphicsPipeline& ConfigureInputAssemblyState(const VkPipelineInputAssemblyStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& SetPrimitiveTopology(const VkPrimitiveTopology& topology);
		VulkanGraphicsPipeline& SetPrimitiveRestartEnable(const VkBool32& prim_restart_enable);
		
		// -------------------------------
		// Tessellation State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureTessellationState(const VkPipelineTessellationStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& SetPatchControlPointCount(const uint32_t& count);

		// -------------------------------
		// Viewport State
		// -------------------------------
		
		VulkanGraphicsPipeline& ConfigureViewportState(const VkPipelineViewportStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& AddViewport(const VkViewport& viewport);
		VulkanGraphicsPipeline& AddScissor(const VkRect2D& scissor);

		// -------------------------------
		// Rasterization State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureRasterizationState(const VkPipelineRasterizationStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& SetRasterizerDiscardEnabled(const VkBool32& rast_discard_enable);
		VulkanGraphicsPipeline& SetPolygonMode(const VkPolygonMode& mode);
		VulkanGraphicsPipeline& SetCullMode(const VkCullModeFlags& mode);
		VulkanGraphicsPipeline& SetFrontFace(const VkFrontFace& front_face);
		VulkanGraphicsPipeline& SetDepthClampEnabled(const VkBool32& depth_clamp_enable);
		VulkanGraphicsPipeline& SetDepthBiasEnabled(const VkBool32& depth_bias_enable);
		VulkanGraphicsPipeline& SetDepthBiasConstantFactor(const float& constant_factor);
		VulkanGraphicsPipeline& SetDepthBiasClamp(const float& bias_clamp);
		VulkanGraphicsPipeline& SetDepthBiasSlopeFactor(const float& slope_factor);
		VulkanGraphicsPipeline& SetLineWidth(const float& line_width);

		// -------------------------------
		// Multisample State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureMultisampleState(const VkPipelineMultisampleStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& SetRasterizationSamples(const VkSampleCountFlagBits& sample_count);
		VulkanGraphicsPipeline& SetSampleShadingEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetMinSampleShading(const float& min_sample_shading);
		VulkanGraphicsPipeline& SetSampleMask(const VkSampleMask& mask);
		VulkanGraphicsPipeline& SetAlphaToCoverageEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetAlphaToOneEnabled(const VkBool32& enable);

		// -------------------------------
		// Depth Stencil State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureDepthStencilState(const VkPipelineDepthStencilStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& SetDepthTestEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetDepthWriteEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetDepthCompareOp(const VkCompareOp& op);
		VulkanGraphicsPipeline& SetDepthBoundsTestEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetStencilTestEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetFrontStencilOpState(const VkStencilOpState& state);
		VulkanGraphicsPipeline& SetBackStencilOpState(const VkStencilOpState& state);
		VulkanGraphicsPipeline& SetMinDepthBounds(const float& min_bounds);
		VulkanGraphicsPipeline& SetMaxDepthBounds(const float& max_bounds);

		// -------------------------------
		// Color Blend State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureColorBlendState(const VkPipelineColorBlendStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& SetLogicOpEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetLogicOp(const VkLogicOp& op);
		VulkanGraphicsPipeline& SetBlendConstants(const float& r, const float& g, const float& b, const float& a);
		VulkanGraphicsPipeline& AddColorAttachment();
		VulkanGraphicsPipeline& SetBlendEnabled(const VkBool32& enable);
		VulkanGraphicsPipeline& SetSrcColorBlendFactor(const VkBlendFactor& blend_factor);
		VulkanGraphicsPipeline& SetDstColorBlendFactor(const VkBlendFactor& blend_factor);
		VulkanGraphicsPipeline& SetColorBlendOp(const VkBlendOp& blend_op);
		VulkanGraphicsPipeline& SetSrcAlphaBlendFactor(const VkBlendFactor& blend_factor);
		VulkanGraphicsPipeline& SetDstAlphaBlendFactor(const VkBlendFactor& blend_factor);
		VulkanGraphicsPipeline& SetAlphaBlendOp(const VkBlendOp& blend_op);
		VulkanGraphicsPipeline& SetColorWriteMask(const VkColorComponentFlags& write_mask);

		// -------------------------------
		// Dynamic State
		// -------------------------------

		VulkanGraphicsPipeline& ConfigureDynamicState(const VkPipelineDynamicStateCreateFlags& create_flags = 0);
		VulkanGraphicsPipeline& AddDynamicState(const VkDynamicState& state);

		// -------------------------------

	private:
		VulkanDevice* m_logical_device;
		VulkanPipelineInfo m_create_info;
		VkPipelineLayoutCreateInfo m_layout_info;
		ConfigurationState m_state;

		std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;
		std::vector<VkDescriptorSetLayout> m_ds_layouts;
		std::vector<VkPushConstantRange> m_push_constants;

		std::vector<VkVertexInputBindingDescription> m_vertex_bindings;
		std::vector<VkVertexInputAttributeDescription> m_vertex_attributes;
		std::vector<VkViewport> m_viewports;
		std::vector<VkRect2D> m_scissors;
		std::vector<VkSampleMask> m_multisample_masks;
		std::vector<VkPipelineColorBlendAttachmentState> m_color_blend_attachments;
		std::vector<VkDynamicState> m_dynamic_states;
	};

	/*class VulkanComputePipeline : public IVulkanPipelineStrategy
	{
	public:
		VulkanComputePipeline();
		virtual ~VulkanComputePipeline() override;

		void Initialize(VulkanDevice* device) override;

		VulkanPipelineInfo& Build() override;

		virtual VulkanComputePipeline& BindShader(
			const VkShaderStageFlagBits& shader_stage,
			const VkPipelineShaderStageCreateFlags& create_flags,
			const std::string& file_path,
			bool is_hlsl = false,
			const char* entry_point = "main"
		) override;

	private:
		VkPipelineShaderStageCreateInfo m_shader_stage;
	};

	class VulkanRayTracingPipeline : public IVulkanPipelineStrategy
	{
	public:
		VulkanRayTracingPipeline();
		virtual ~VulkanRayTracingPipeline() override;

		void Initialize(VulkanDevice* device) override;

		VulkanPipelineInfo& Build() override;

		virtual VulkanRayTracingPipeline& BindShader(
			const VkShaderStageFlagBits& shader_stage,
			const VkPipelineShaderStageCreateFlags& create_flags,
			const std::string& file_path,
			bool is_hlsl = false,
			const char* entry_point = "main"
		) override;

	private:
		std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;
	};*/
}