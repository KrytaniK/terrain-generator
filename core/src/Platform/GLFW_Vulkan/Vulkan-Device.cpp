#include <macros/AurionLog.h>

#include <utility>
#include <vector>
#include <set>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;

VulkanDevice VulkanDevice::Create(const VkInstance& instance, const VulkanDeviceConfiguration& reqs)
{
	VulkanDevice device;

	// Choose a suitable physical device
	{
		std::vector<VkPhysicalDevice> physical_devices;

		// Get all physical devices
		{
			// Query physical device count
			uint32_t deviceCount = 0;
			if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS)
			{
				AURION_ERROR("Failed to enumerate physical devices...");
			}

			// Ensure we have valid devices
			if (deviceCount == 0)
			{
				AURION_ERROR("Could not find any GPUs with Vulkan support!");
			}

			// Resize container
			physical_devices.resize(deviceCount);

			// Retrieve all physical devices
			vkEnumeratePhysicalDevices(instance, &deviceCount, physical_devices.data());
		}

		// Check for a device that meets the requirements
		for (const auto& physical_device : physical_devices)
		{
			if (device.physical_device != VK_NULL_HANDLE)
				continue;

			if (VulkanDevice::MeetsRequirements(physical_device, reqs))
				device.physical_device = physical_device;
		}

		// Bail early if a valid device wasn't found
		if (device.physical_device == VK_NULL_HANDLE)
		{
			AURION_ERROR("[VulkanDevice::Create] Failed to find a suitable GPU with the provided requirements.");
			return std::move(device);
		}
	}

	// Using the pNext chain from the requirements, retrieve device properties/features.
	device.features = reqs.features;
	device.properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	vkGetPhysicalDeviceProperties2(device.physical_device, &device.properties);
	vkGetPhysicalDeviceFeatures2(device.physical_device, &device.features);

	// Query for device queue indices (Graphics & Compute Only)
	{
		// Query for supported queue families
		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device, &family_count, nullptr);

		// Get all queue families
		std::vector<VkQueueFamilyProperties> queueFamProps(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device, &family_count, queueFamProps.data());

		// Query queue family support for the chosen physical device
		uint32_t i = 0;
		for (const VkQueueFamilyProperties& props : queueFamProps)
		{
			// Check for support for graphics operations
			if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				device.graphics_queue_index = i;

			// Check for support for compute operations
			if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
				device.compute_queue_index = i;

			i++;
		}
	}

	// Create logical device handle
	{
		// Attach relevant queues
		float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queue_infos;
		std::set<uint32_t> queue_indices;

		if (device.graphics_queue_index.has_value())
			queue_indices.emplace(device.graphics_queue_index.value());

		if (device.compute_queue_index.has_value())
			queue_indices.emplace(device.compute_queue_index.value());

		// Attach each unique queue create info
		for (uint32_t index : queue_indices)
		{
			VkDeviceQueueCreateInfo queue_info{};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = index;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &queuePriority;

			queue_infos.push_back(queue_info);
		}

		// Initialize logical device creation structure
		VkDeviceCreateInfo device_info{};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
		device_info.pQueueCreateInfos = queue_infos.data();

		// Enable required extensions
		device_info.enabledExtensionCount = static_cast<uint32_t>(reqs.extensions.size());
		device_info.ppEnabledExtensionNames = reqs.extensions.data();

		// Enable required features
		device_info.pNext = &reqs.features; // Features for 1.1+

		// Create logical device
		if (vkCreateDevice(device.physical_device, &device_info, nullptr, &device.handle) != VK_SUCCESS)
		{
			AURION_ERROR("[VulkanDevice::Create] Failed to create the logical device!");
			return std::move(device);
		}
	}

	// Attach VkInstance if logical device creation succeeded
	device.vk_instance = instance;

	// VMA Allocator
	{
		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.physicalDevice = device.physical_device;
		allocatorInfo.device = device.handle;
		allocatorInfo.instance = device.vk_instance;
		allocatorInfo.flags = reqs.allocator_flags;

		if (vmaCreateAllocator(&allocatorInfo, &device.allocator) != VK_SUCCESS)
		{
			AURION_ERROR("[VulkanDevice::Create] Allocator initialization failed.");
			return std::move(device);
		}
	}

	// Get graphics/compute queues
	{
		VkDeviceQueueInfo2 queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
		queue_info.pNext = nullptr;
		queue_info.flags = 0;
		queue_info.queueIndex = 0;

		// Retrieve Graphics Queue
		if (device.graphics_queue_index.has_value())
		{
			queue_info.queueFamilyIndex = device.graphics_queue_index.value();
			vkGetDeviceQueue2(device.handle, &queue_info, &device.graphics_queue);

			if (device.graphics_queue == VK_NULL_HANDLE)
				AURION_ERROR("[VulkanDevice::Create] Failed to fetch graphics queue with index %d", queue_info.queueFamilyIndex);
		}

		// Retrieve Compute Queue
		if (device.compute_queue_index.has_value())
		{
			queue_info.queueFamilyIndex = device.compute_queue_index.value();
			vkGetDeviceQueue2(device.handle, &queue_info, &device.compute_queue);

			if (device.compute_queue == VK_NULL_HANDLE)
				AURION_ERROR("[VulkanDevice::Create] Failed to fetch graphics queue with index %d", queue_info.queueFamilyIndex);
		}
	}

	return std::move(device);
}

bool VulkanDevice::MeetsRequirements(const VkPhysicalDevice& physical_device, const VulkanDeviceConfiguration& reqs)
{
	// Check physical device properties
	VkPhysicalDeviceProperties2 device_props{};
	device_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	// Grab physical device properties
	vkGetPhysicalDeviceProperties2(physical_device, &device_props);

	// If the device type isn't what's required, bail
	if (device_props.properties.deviceType != reqs.device_type)
		return false;

	// Vulkan 1.1 temp features struct
	VkPhysicalDeviceVulkan11Features features11{};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	// Vulkan 1.2 temp features struct
	VkPhysicalDeviceVulkan12Features features12{};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	// Vulkan 1.3 temp features struct
	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

	// Vulkan 1.4 temp features struct
	VkPhysicalDeviceVulkan14Features features14{};
	features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;

	// Copy device feature requirements
	VkPhysicalDeviceFeatures2 device_features_copy{};
	device_features_copy.features = reqs.features.features;
	device_features_copy.sType = reqs.features.sType;

	// Deep copy all pNext members
	void* req_pNext = reqs.features.pNext;
	void* avail_pNext = device_features_copy.pNext;
	while (req_pNext != nullptr)
	{
		VkBaseOutStructure* req_base = (VkBaseOutStructure*)req_pNext;

		switch (req_base->sType)
		{
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES:
			{
				avail_pNext = &features11;
				break;
			}
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES:
			{
				avail_pNext = &features12;
				break;
			}
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
			{
				avail_pNext = &features13;
				break;
			}
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES:
			{
				avail_pNext = &features14;
				break;
			}
			default:
			{
				AURION_CRITICAL("[VulkanDevice::MeetsRequirements] Unsupported sType member! Value: %d", req_base->sType);
				break;
			}
		}

		req_pNext = req_base->pNext;
		avail_pNext = ((VkBaseOutStructure*)avail_pNext)->pNext;
	}

	// Populate the full pNext chain of the deep copy
	vkGetPhysicalDeviceFeatures2(physical_device, &device_features_copy);

	// Check for at least the required core features (memory comparison since they're all VkBool32)
	size_t core_feature_count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
	const VkBool32* required_core_features = reinterpret_cast<const VkBool32*>(&reqs.features.features);
	VkBool32* available_core_features = reinterpret_cast<VkBool32*>(&device_features_copy.features);
	for (size_t i = 0; i < core_feature_count; i++)
	{
		// We only care about required features that have been set to true.
		if (required_core_features[i] == VK_FALSE)
			continue;

		// If the required feature isn't supported, this device doesn't meet the requirements
		if (available_core_features[i] == VK_FALSE)
		{
			AURION_ERROR(
				"[VulkanDevice::MeetsRequirements] Device Failed to meet core requirements. Feature Index: %d",
				i
			);
			return false;
		}
	}

	// Traverse the pNext chain and check each specified version's required features
	// (memory comparison since they're all VkBool32, HOWEVER: Offset by the sType and pNext
	//	members, since they occur first)
	void* required_pNext = reqs.features.pNext;
	void* available_pNext = device_features_copy.pNext;
	while (required_pNext != nullptr && available_pNext != nullptr)
	{
		VkBaseOutStructure* required_base = (VkBaseOutStructure*)required_pNext;
		VkBaseOutStructure* available_base = (VkBaseOutStructure*)available_pNext;

		// Ensure the types of the required structure and deep copy match exactly. Bail if they don't
		if (required_base->sType != available_base->sType)
		{
			AURION_CRITICAL(
				"[VulkanDevice::MeetsRequirements] Mismatch of Device Feature's pNext member!\n\tRequired Type: %d\n\tCopy Type: %d",
				required_base->sType,
				available_base->sType
			);
			return false;
		}

		// Calculate the offset, feature count, and array start for determined structure type
		size_t feature_count = 0;
		size_t offset = 0;
		const VkBool32* required_features = nullptr;
		const VkBool32* available_features = nullptr;
		switch (required_base->sType)
		{
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES:
			{
				// Calculate offset & feature cont
				offset = sizeof(VkStructureType) + sizeof(void*);
				feature_count = (sizeof(VkPhysicalDeviceVulkan11Features) - offset) / sizeof(VkBool32);
				break;
			}
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES:
			{
				offset = sizeof(VkStructureType) + sizeof(void*);
				feature_count = (sizeof(VkPhysicalDeviceVulkan12Features) - offset) / sizeof(VkBool32);
				break;
			}
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
			{
				offset = sizeof(VkStructureType) + sizeof(void*);
				feature_count = (sizeof(VkPhysicalDeviceVulkan13Features) - offset) / sizeof(VkBool32);
				break;
			}
			case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES:
			{
				offset = sizeof(VkStructureType) + sizeof(void*);
				feature_count = (sizeof(VkPhysicalDeviceVulkan14Features) - offset) / sizeof(VkBool32);
				break;
			}
			default:
			{
				AURION_CRITICAL("[VulkanDevice::MeetsRequirements] Failed to check device requirements: Unsupported VkStructureType!");
				return false;
				break;
			}
		}

		// Generate a pointer to the starting feature in the required structure
		unsigned char* req_byte_arr = (unsigned char*)required_base;
		req_byte_arr = (req_byte_arr + offset);

		// Generate a pointer to the starting feature in the required structure
		unsigned char* avail_byte_arr = (unsigned char*)available_base;
		avail_byte_arr = (avail_byte_arr + offset);

		// Reinterpret each as a vulkan boolean array
		required_features = reinterpret_cast<const VkBool32*>(req_byte_arr);
		available_features = reinterpret_cast<const VkBool32*>(avail_byte_arr);

		// Check all required features
		for (size_t i = 0; i < feature_count; i++)
		{
			// We only care about required features that have been set to true.
			if ((required_features[i] == VK_TRUE) && (available_features[i] == VK_FALSE))
			{
				AURION_ERROR(
					"[VulkanDevice::MeetsRequirements] Device Failed to meet requirements. Structure Type: %d, Feature Index: %d", 
					required_base->sType, 
					i
				);
				return false;
			}
		}

		required_pNext = required_base->pNext;
		available_pNext = available_base->pNext;
	}

	// If all checks passed, this device is a solid choice
	return true;
}
