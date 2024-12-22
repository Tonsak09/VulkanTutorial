// Include for having glfw - vulkan relations 
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include <optional>

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
        // The device we will use. It is destroyed implicitly 
        // when the instance is destroyed 
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    
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
        return indicies.IsComplete();



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

        bool IsComplete()
        {
            return graphicsFamily.has_value();
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
        PickPhysicalDevice();
    }

    void MainLoop() 
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void Cleanup() 
    {
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

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