// Include for having glfw - vulkan relations 
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <set>

#include <cstdint> // For uin32_t
#include <limits> // For numerical limits 
#include <algorithm> // for std::clamp 

#include <optional>

#include <fstream>

class HelloTriangleApplication {
public:
    void Run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::vector<const char*> validationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    // Whether or not we use validation to check for common errors
    // instead of crashing
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif // NDEBUG

    VkDebugUtilsMessengerEXT debugMessenger;


    // The device we will use. It is destroyed implicitly 
    // when the instance is destroyed 
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device; // Logic device 
    VkQueue graphicsQueue; // Implicitly cleaned 

    // Object and usage is platform agnostic, its creation is not 
    // mandatory since vulkan can operate offline 
    VkSurfaceKHR surface; 
    VkQueue presentQueue;


    // Need to make sure that there is swap chain support. 
    // This is because Vulkan does not need to work on a 
    // device that has a display output, like a rendering 
    // server. 

     // Does our device extend swapchain capabilities 
    const std::vector<const char*> deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes; 
    };
    

    VkSwapchainKHR swapChain;
    // Views let us access the images 
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkImage> swapChainImages; 
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    // Have to choose which framebuffer to use for presentation
    // and then other requirements 
    std::vector<VkFramebuffer> swapChainFramebuffers; 

    VkCommandPool commandPool;
    // A buffer for each frame in flight 
    std::vector<VkCommandBuffer> commandBuffers; // Cleaned up automatically 
    std::vector<VkSemaphore> imageAvailableSemaphores; 
    std::vector<VkSemaphore> renderFinishedSemaphores; 
    std::vector<VkFence> inFlightFences;

    // How many frames can be processed concurrently 
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0; 

private: // Vukan helpers 
    
    #pragma region Instance Creation 

    void CreateInstance()
    {
        // If debugging then check if validation layer is supported 
        if (enableValidationLayers && !CheckValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not avaliable");
        }
        
        // Set up information about our instance of vulkan 
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Tells our Vulkan driver which global extension 
        // and validation layers we want to use 
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        auto extensions = GetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        // Additional info is using validation layers 
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledExtensionCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance))
        {
            throw std::runtime_error("Failed to create instance!");
        }

    }

    /// <summary>
    /// Gets the required extensions for the current instance that
    /// connects it to GLFW 
    /// </summary>
    std::vector<const char*> GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Check if validation layers are required 
        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    #pragma endregion 

    #pragma region Validation Layers

    /// <summary>
    /// Callback for printing validation layer messages 
    /// </summary>
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) // Contains a pointer to oursetup and lets us set data to it 
    {
        // Note: We can use different logic based on the message severity 
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT : Informational message like the creation of a resource
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : Message about behavior that is not necessarily an error, but very likely a bug in your application
        //      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : Message about behavior that is invalid and may cause crashes

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            // Important to show 
        }

        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    /// <summary>
    /// Checks if all requested layers are avaliable 
    /// </summary>
    bool CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> avaliableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, avaliableLayers.data());

        // Iterate through all validation types to see if 
        // the desired validation layer is there VK_LAYER_KHRONOS_validation
        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : avaliableLayers)
            {
                // String values are identical 
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>
    /// Sets up a messenger that gives us information
    /// from our vulkan instance 
    /// </summary>
    void SetupDebugMessenger()
    {
        // Don't setup if not debugging 
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessangerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }

    }

    /// <summary>
    /// Because our function vkCreateDebugUtilsMessenerEXT is an extension
    /// function it is not automatically loaded. We need to manually look
    /// it up and return it as a VkResult 
    /// </summary>
    VkResult CreateDebugUtilsMessangerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        // Search for vkCreateDebugUtilsMessenerEXT
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
            "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    /// <summary>
    /// Our messenger must be destroyed. vkDestroyDebugUtilsMessengerEXT 
    /// is not automatically loaded so we must do it manually before its
    /// call
    /// </summary>
    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
            "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    /// <summary>
    /// Populates a messenger 
    /// </summary>
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
    }

    #pragma endregion

    #pragma region Physical Devices and Queue Families 

    /// <summary>
    /// Choose a graphics card that supports the 
    /// features we require
    /// </summary>
    void PickPhysicalDevice()
    {
        // Get amount of devices avaliable 
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        // Iterate through all avaliable GPUs to see if 
        // any are valid. Picks the first valid 
        for (const auto& device : devices)
        {
            if (IsDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    /// <summary>
    /// Checks whether the device is appropriate for 
    /// our requirements 
    /// </summary>
    bool IsDeviceSuitable(VkPhysicalDevice device)
    {
        // NOTE: As an alternative we could give a score
        //       for each feature that we desire and pick
        //       the one with the highest

        QueueFamilyIndicies indicies = FindQueueFamilies(device);

        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            // Check if both formats and present modes
            // are not empty lists 
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indicies.IsComplete() && extensionsSupported && swapChainAdequate;


 


        // Following code searches for a device that has a discrete GPU
        // and geometry shader features.

        // Just because our device has the capabilities to work with the
        // features we want, it does not mean we have access to those 
        // queues just yet. We need access to the family the designates
        // rules to those commands which is why we have functionallity that
        // checks specifically for that above. 

        /*
         // Properties include name, type and support of vulkan version 
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Features include texture compression, 64 bit floats 
        // and multi viewport rendering 
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return deviceProperties.deviceType == 
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceFeatures.geometryShader;
        */
       
    }


    /// <summary>
    /// Bundle of our queues 
    /// </summary>
    struct QueueFamilyIndicies
    {
        // Note: We use an optional because it allows
        //       us to check if there is actually a 
        //       value contained. More readable than
        //       std::pair<type, bool> 

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily; 

        bool IsComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    /// <summary>
    /// Searches for the queue families this device 
    /// has access to 
    /// </summary>
    /// <returns></returns>
    QueueFamilyIndicies FindQueueFamilies(VkPhysicalDevice device)
    {
        // Searching for graphics queue family 
        QueueFamilyIndicies indicies;

       
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // Properties include type of operations supported, num of
        // queues able to be created from family, etc 
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // Sarch for family with graphics queue 
        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            // Is there a family that supports the surface? 
            if (presentSupport)
            {
                indicies.presentFamily = i;
            }

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                // Store id of graphics family 
                indicies.graphicsFamily = i;
            }

            if (indicies.IsComplete())
            {
                break;
            }

            i++;
        }

        return indicies;
    }

    #pragma endregion

    #pragma region Logic Device and Queues

    /// <summary>
    /// Generate a logic device which allows us to interface
    /// with 
    /// </summary>
    void CreateLogicalDevice()
    {
        QueueFamilyIndicies indices = FindQueueFamilies(physicalDevice);
        
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        
        float queuePriority = 1.0;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            // Note: This creates a queue for both graphics queue along with
            //       the present queue 

            // Generate a queueCreateInfo specifically for 
            // for the graphics family 
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
            queueCreateInfo.queueCount = 1;

            // Note: We can create a small number of queues
            //       for our desired queue family but we will
            //       most likely not need more than one 
            // 
            // We can assign priority to queue which influences
            // scheduling. Necessary even for a single queue 
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

       

       

        // What features will we be requesting from our
        // device queue 
        VkPhysicalDeviceFeatures deviceFeatures{}; 


        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        
        // Fill out queue info 
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        // Attach features 
        createInfo.pEnabledFeatures = &deviceFeatures;


        // Specify any device specific extensions 
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // Connect validation layers for debugging 
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }


        // Create device 
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        // Create handle to interface with graphics queue
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    #pragma endregion

    #pragma region Window Surface

    void CreateSurface()
    {
        // Note: Lets us access the platform's window 

        // This seems to be the setup for creating a surface for windows32
        /*VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }*/


        // Sets up the window surface using GLFW 
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    #pragma endregion

    #pragma region Swap Chain

    // Note: Much of the logic that is done to stup
    //       the swapchain is in IsDeviceSuitable.
    //       These functions simply help us execute 
    //       our setup process 

    /// <summary>
    /// Checks whether the physical device can use the swapchain
    /// to display textures 
    /// </summary>
    /// <param name="device"></param>
    /// <returns></returns>
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        // Note: I am note really surer why we are calling the same
        //       function twice and applying the extension count 
        //       
        //       It seems we first get the total amount of extensions
        //       and make a vector to hold them after with the proper
        //       amount of prepared space 

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
        // Removes any extension that this device has 
        for (const auto& extension : availableExtensions)
        {
            // TODO: More efficient way to replace erase? 
            requiredExtensions.erase(extension.extensionName);
        }

        // Returns whether or not there are extensions
        // that support the current physical device 
        return requiredExtensions.empty();
    }

    /// <summary>
    /// Populates a struct that holds the information
    /// about the swapchain capabilities of this device 
    /// </summary>
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;


        // Get the supported surface formats 
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
            

        // Resize to hold all avaliable formats 
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    /// <summary>
    /// Picks the desired surface formats necessary for the program 
    /// </summary>
    /// <param name="availableFormats"></param>
    /// <returns></returns>
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        // Iterates through and tries to find a format that
        // has the SRGB format and color space 
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        // Here we may want to return the best ranked
        // format. As a default we'll just do the first
        // in the array 

        return availableFormats[0];
    }

    /// <summary>
    /// Chooses the rate at which we update and present
    /// our swapchain 
    /// </summary>
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& avaliablePresentModes)
    {
        // VK_PRESENT_MODE_IMMEDIATE_KHR        Images transferred to screen right away. May
        //                                      result in tearing 
        // VK_PRESENT_MODE_FIFO_KHR             Updates a queue that displays based on FIFO.
        //                                      If the queue is full then the program has to
        //                                      wait. The refresh is a "vertical blank"
        // VK_PRESENT_MODE_FIFO_RELAXED_KHR     Only difference is that if the queue is empty
        //                                      then the image is immediatly added and does
        //                                      not wait for a vertical blank 
        // VK_PRESENT_MODE_MAILBOX_KHR          A variation on FIFO mode where a full queue
        //                                      results in frames in the queue being replaced. 
        //                                      Also known as "triple buffering" 

        // Since VK_PRESENT_MODE_FIFO_KHR is the only mode guaranteed to 
        // be avaliable we will just use that as a base 

        for (const auto& availablePresentMode : avaliablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /// <summary>
    /// Set the resolution of the swap chain images 
    /// </summary>
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        // The image resolutions almost always match exactly
        // with the resolution of the window in pixels. 

        // Not all monitors use pixels so we have to be a little 
        // trickey when equating the two 

        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent =
            {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // Clamps width and height to largest and smallest
            // allowed extents 
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    /// <summary>
    /// Create a swap chain from using the current
    /// device's capabilities 
    /// </summary>
    void CreateSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

        // We sometimes may have to wait for internal operations to get
        // another image to render to. So, we simply add another in case 
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface; 

        // Note: For image use if we want to perform postt processes 
        //       we may want to set it to VK_IMAGE_USAGE_TRANSFER_DST_BIT

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // How many layers each image consists of 
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


        // Need to coordinate whether our swapchains will be used
        // across multiple queue families. This can happen if our
        // graphics queue family is different from the presentation
        // queue. 

        // VK_SHARING_MODE_EXCLUSIVE    An image is owned by one queue family at a time.
        //                              Must be explicitly transferred before using 
        //                              another queue family 
        // VK_SHARING_MODE_CONCURRENT   Images can be used across multiple queue families
        //                              without explicit ownership transfers 

        QueueFamilyIndicies indices = FindQueueFamilies(physicalDevice);
        uint32_t QueueFamilyIndicies[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            // Different families 
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = QueueFamilyIndicies;
        }
        else
        {
            // Same family 
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; 
            createInfo.pQueueFamilyIndices = nullptr;
        }

        // Should transforms be applied to the image? 
        // Default is currentTransform 
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        // Should alpha be used for blending 
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // We may sometimes need to recreate the swap chain. Resizing and such 
        // Default is VK_NULL_HANDLE
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        // Connect to the array of vkimages we have 
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        // We now finally have a set of images we can draw to! 
    }


    #pragma endregion

    #pragma region Image Views 

    void CreateImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];

            // How should the data be interpreted? 
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            // Allow for color swizzling of each component 
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // Image purpose and how it should be accessed 
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image views!"); 
            }

            // View is ready for texture use but not quite ready
            // to be used a render target yet! 
        }
    }

    #pragma endregion

    #pragma region Graphics Pipeline

    /// <summary>
    /// Initilizes the graphics pipeline 
    /// </summary>
    void CreateGraphicsPipeline()
    {
        // TODO: Automate the process of pipeline creation 

        auto vertShaderCode = ReadFile("Shaders/vert.spv");
        auto fragShaderCode = ReadFile("Shaders/frag.spv");

        VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);


        // To use the shader we need to assign them to their 
        // repsepctive pipeline stage 


        // Note: The specilized info allows us to sepcify values for 
        //       shader constants. More efficient than configuring 
        //       variables during render time 

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo shaderStages[]{ vertShaderStageInfo, fragShaderStageInfo };



        // ------------ Dynamic State ------------

        // Allows us to edit parts of the pipeline during the 
        // runtime in a limited amount 
        std::vector<VkDynamicState> dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        // Now these values will be ignored in the pipeline but
        // its requires us to manually input it in 
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // ------------ Vertex Input ------------

        // Note: Describes how our data will be passed to the
        //       vertex shader. 
        //
        //       Bindings: Spacing between data. This could be 
        //                 per vertex or 

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // ------------ Input Assumbly ------------

        // Note: Defines what kind of geometry is being drawn
        //       and if primitive restart should be active. 
        //
        //      VK_PRIMITIVE_TOPOLOGY_POINT_LIST        
        //      VK_PRIMITIVE_TOPOLOGY_LINE_LIST         
        //      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP        
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST     
        //      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP    
        //
        //      primitiveRestart: Makes it posible to break up
        //                        lines and triangles with the 
        //                        strip modes. 

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // ------------ Input Assumbly ------------

        // Note: This is used to define the viewport that the frame
        //       buffer will be rendered to.

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Draws the entire framebuffer 
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;


        // ------------ Rasterizer ------------

        // Note: This is where fragments are colored by the gragment 
        //       shader. Also where depth testing, face culling, and 
        //       scissor tests are done. 

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        // Keeps within the defined range we set (in the viewport?)
        rasterizer.depthClampEnable = VK_FALSE;

        // Sets whether geometry will pass through the rasterizer
        // stage. Disables output to framebuffer 
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        // How fragments get generated 
        //      VK_POLYGON_MODE_FILL    
        //      VK_POLYGON_MODE_LINE    
        //      VK_POLYGON_MODE_POINT    

        // QUESTION: What is the difference between how meshes are used
        //           here in the rasterizer versus Input Assembly 

        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f; // Size of line of fragments 
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        // Depth data 
        //      Allows us to manipulate depth values by adding a const
        //      or change a bais by the slope (The hell is slope?) 
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;


        // ------------ Multisampling ------------

        // Note: Allows us sample multiple times for a single buffer?
        //       Used for anti-aliasing 

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // ------------ Depth & Stencil ------------

        // Note: We can define a depth/stencil struct if we are using
        //       those buffers 


        // ------------ Color Blending ------------

        // Note: Once the fragment has returned a color we need to combine
        //       it with the colors already present on the framebuffer.
        //       Does two blending either mixing or bitwise 

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        
        // TODO: Set a setting to allow us to blend color 
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
        // If we want blending to be done via bitwise calcs then 
        // set logi op to true 
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;


        // ------------ Pipeline Creation ------------

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Another way to add dynamic values 
        pipelineLayoutInfo.pPushConstantRanges = nullptr; 


        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }



        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages; 

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = pipelineLayout;

        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0; // Index to subpass

        // Lets us derrive a new pipeline from an existing pipeline 
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        // Can take multiple infos and create multriple pipelines 
        // Can store pipeline cache for reusing 
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        // Can be cleaned up after passing it to the graphics pieline 
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    }

    /// <summary>
    /// Reads a file at the given path and simply returns it
    /// as a vector of its chars 
    /// </summary>
    static std::vector<char> ReadFile(const std::string& fileName)
    {
        // ate:     Start reading at end of file
        // binary:  Read the file as binary avoiding text transformations 
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer; 
    }

    /// <summary>
    /// Converts byte code into a vulkan ussable shader 
    /// module
    /// </summary>
    VkShaderModule CreateShaderModule(const std::vector<char>& code)
    {

        // Note: We are getting our code data as a char* instead of
        //       byte data so we need to reinterpret its data type 

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule; 
    }

    /// <summary>
    /// Tell the pipeline about frame buffer attachments that 
    /// will be used 
    /// </summary>
    void CreateRenderPass()
    {
        // Attachment types
        //  pInputAttachments       Attachments that are read from a shader
        //  pResolveAttachments     Attachments used for multrisampling color
        //                          attachments 
        //  pDepthStenvilAttachment Attachment for depth and stencil data
        //  pPreserveAttachments    Attachments that are not used by this subpass
        //                          but for which the data must be preserved 



        // This format should match the format of the swap chain images
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // What to do with the data before and after rendering 
        //
        //  VK_ATTACHMENT_LOAD_OP_LOAD          Preserve existing contents of attachment
        //  VK_ATTACHMENT_LOAD_OP_CLEAR         Clears the value to a constant at the start
        //  VK_ATTACHMENT_LOAD_OP_DONT_CARE     Existenting contents are undefined; we don't
        //                                      care about them 
        //  VK_ATTACHMENT_LOAD_OP_STORE         Rendered contents we be store and used later
        //  VK_ATTACHMENT_STORE_OP_DONT_CARE    Contents in framebuffer become undefined after 
        //                                      rendering operations
       
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // Index in attachment description array 
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        // Can be comopute so must be explicit this is graphics subpass 
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // Index referneced in fragment shader 
        // EX: layout(location = 0) out vec4 outColor
        subpass.colorAttachmentCount = 1; // Count not index! 
        subpass.pColorAttachments = &colorAttachmentRef;


        // Subpass dependencies 
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;



        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment; // Array if multi
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;







        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    #pragma endregion

    #pragma region Drawing

    /// <summary>
    /// Generates the frame buffers that will be used 
    /// </summary>
    void CreateFrameBuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        // Iterate through image views and create frame buffers
        // for each of them 
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            // Need to define which render passes this swapchain is compatible with 
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
    }

    // Note: We do not send commands on a function by function basis
    //       but rather combine them all into one buffer object to 
    //       send all at once 

    /// <summary>
    /// Setup the command pool we will be sending to vulkan 
    /// </summary>
    void CreateCommandPool()
    {
        QueueFamilyIndicies  queueFamilyIndices = FindQueueFamilies(physicalDevice);

        // Note: there are two types of command pools
        //  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT                Hint command buffers are rerecorded often with new commands
        //  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT     Commands buffers are recorded individually 

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        // Our command buffer must submitted to a device queue
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    /// <summary>
    /// Contructs the comman buffers which our command pool can access 
    /// </summary>
    void CreateCommandBuffers()
    {
        // Buffer Type:
        //  VK_COMMAND_BUFFER_LEVEL_PRIMARY     Can be submitted to queue for execution but cannot be
        //                                      called from secondary buffers 
        //  VK_COMMAND_BUFFER_LEVEL_SECONDAY    Cannot be submitted direectly but can be called from 
        //                                      prrimary command buffers 

        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();


        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    /// <summary>
    /// Writes the commands we want to executte in our command buffer 
    /// </summary>
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        // Flags determine how we are using this command buffer
        //  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT         Command buffer will be rerecorded after 
        //                                                      executing once 
        //  VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT    This is a secondary command buffer existing 
        //                                                      within a single render pass
        //  VK_COMMAND_BUFFER_USAGE_SIMULATEOUS_USE_BIT         Can be resubmitted while it is also already 
        //                                                      pending execution 

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        // Only relevant to secondary command buffers 
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }


        // ------------ Starting Render Pass ------------

        // Not really sure if this is where it goes????

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        // Current swap chain frame buffer 
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

        // Size of render area 
        // Any pixel outside the region will be undefinedd values 
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;
    
        // Setup default values when clearing the screen 
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // How drawing commands will be provided 
        //  VK_SUBPASS_CONTENTS_INLINE                      Embedded into the primary command buffer with no
        //                                                  secondary command buffer being executed 
        //  VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS   Render pass commands will be executed from secondary
        //                                                  command buffers

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
        // ------------ Basic Drawing Commands ------------
        
        // Binding the command buffer to the graphics pipeline 
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


        // Note: We have already told the pipeline what information we need to send 
        //       so we are simply setting them up here before sending them over 

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
        vkCmdDraw(
            commandBuffer, 
            3,  // vertexCount
            1,  // instanceCount
            0,  // Offset to first vertex 
            0   // offset to first instance 
        );

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    /// <summary>
    /// Updates what is being rendered! 
    /// </summary>
    void DrawFrame()
    {
        // Outline of frame
        //  Wait for previous frame 
        //  Get image from swap chain
        //  Record command buffer that draws scene onto image
        //  Submit recorded command buffer
        //  Present swapchain image

        // Sempahores are either active or not. Semaphore A will signal 
        // semaphore B with it is done. 

        // If the host needs to know when the GPU has finished something, 
        // we use a fence. We essentially can cause the code to wait 
        // where there is a fence 

        // We want to wait for all the fences to return true 
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        // Acquire an image from the swap chain
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Record command buffer
        //  Second param is a flag for resting the command buffer 
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        // Submit the command buffer 
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] }; // Adds semaphore 
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores; 
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        // What to signal after command buffers have finished execution 
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores; 

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer");
        }


        // Presentation 
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[]{ swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex; 

        // Can be used to hold an array of VkResult to check if each
        // swap chain is successful 
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    /// <summary>
    /// Creates the vulkan specific semaphores 
    /// </summary>
    void CreateSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
            
        }

        //VkSemaphoreCreateInfo semaphoreInfo{};
        //semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        //
        //VkFenceCreateInfo fenceInfo{};
        //fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        //fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        //
        //// Similiar creation pattern for semaphores and fences 
        //if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        //    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        //    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
        //{
        //    throw std::runtime_error("Failed to create semaphores!");
        //}
    }


    #pragma endregion

private: // Main functions 
    void InitWindow()
    {
        glfwInit();

        // Tell application to not create an OpenGL context 
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);


        // Generate window 
        //      Fourth parameter lets us choose a montitor to open to
        //      Fifth parameter is only relevant to OpenGL
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void InitVulkan() 
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFrameBuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void MainLoop() 
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            DrawFrame();
        }

        // Wait for our device since they are async
        vkDeviceWaitIdle(device);
    }

    void Cleanup() 
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        
        
        

        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        // Once we have multiple pipelines we can destroy them all here 
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        vkDestroyRenderPass(device, renderPass, nullptr);


        // Since we created the image views we need to manually 
        // destroy all of them 
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}