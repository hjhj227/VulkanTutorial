#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32

#include<GLFW/glfw3.h>
#include<GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <cstring>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;       // 스왑체인 할 이미지의 최대/최소 개수, 이미지의 최대/최소 너비 & 높이 값
	std::vector<VkSurfaceFormatKHR> formats;     // 픽셀 형식, 색 공간 등
	std::vector<VkPresentModeKHR> presentModes;  // 이미지 표시 방법
};

class HelloTriangleApplication {
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    // 디바이스 큐는 디바이스가 소멸하면 자동이로 함께 소멸되어 따로 cleanup 처리 해줄 필요는 없음~
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        
        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
        }

        return details;
    }

    // 최적의 스왑 체인을 생성하기 위해 적절한 세팅 값들을 골라줘야 한다.
    // Surface Format (color depth)
    // Presentation Mode (conditions for "swapping" images to the screen) 
	// Swap Extent (resolution of images in swap chain)
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
				return availableFormat;
            }
        }

		return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
		return VK_PRESENT_MODE_FIFO_KHR;
    }

    void initWindow()
    {
        glfwInit();

        // glfw는 원래는 openGL을 위해 만들어진 라이브러리이기 때문에 뒤에 OpenGL 전용으로 생성되지 않도록 힌트를 주는 함수를 호출해야함
        // 뒤에서 알아가지고 OpenGL 전용이 아니게 준비할 수 있도록
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // 3번째 인자는 윈도우의 타이틀, 4번쨰 인자는 윈도우가 열릴 모니터, 5번째 인자는 OpenGL에서만 사용하는 인자
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMssenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents(); // X 버튼 눌림과 같은 이벤트가 발생했는지 체크하고 발생했으면 처리
        }
    }

    void cleanup() {
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(instance, surface, nullptr); // GLFW는 window sufrace를 제거하는 함수는 제공하지 않으므로 원래 API를 사용해서 제거해 준다.
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        // VkApplicationInfo 는 만들고 있는 어플리케이션에 대한 정보를 담는 구조체이다.
        // 이 구조체는 필수적으로 작성해야하는 것은 아니지만, 드라이버에게 최적화하기 유용한 정보들을 제공한다.
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        //벌칸에 있는 구조체들은 상당수가 sType이라는 멤버 변수를 통해 타입을 명시하도록 한다.
        //아래에 오는 구조체는 필수적으로 Vulkan Driver에 전달해야하는 정보이며 사용하고자하는 global extention과 validation layer 정보를 담고 있다.
        //여기서 Global은 특정 기기(장치가) 아닌 프로그램 전체에 적용됨을 의미한다.

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void setupDebugMssenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface()
    {
        // glfwCreateWindowSurface 함수에 필요한 인자는 VkInstance, GLFW window 포인터, 커스텀 allocator 그리고 포인터다
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 진단한 내용을 담은 메세지
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT : 리소스 생성과 같은 내용을 전달하는 정보성 메세지, 에러 잠재성을 띄지는 않으므로 위 변수에 포함되지 않았다.
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : 확실한 에러는 아니지만, 작성 중인 프로그램에 잠재적 문제가 있을 수 있음을 알리는 경고 메세지
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EX: 실제 에러가 발생했음을 알리는 메세지

        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        //messageType 파라미터는 다음과 같은 값들을 가질 수 있다.
        //VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: 세분화나 성능과 관련 없는 이벤트가 발생했을 때의 메세지
        //VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: 사양에 어긋나거나 실제 문제의 소지가 있는 경우에 대한 메세지
        //VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: 벌칸에 최적이 아닐 수 있는 내용에 대한 메세지

        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;

    }

    // 사용할 그래픽 카드 고르는 함수
    void pickPhysicalDevice()
    {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        uint32_t deviceCount = 0;

        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // VkDeviceQueueCreateInfo 구조체는 하나의 큐 패밀리가 가질 큐 개수를 지정한다.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        // vulkan은 커맨드 버퍼를 스케줄링하기 위해 큐에 0에서 1사이의 값으로 우선순위를 지정할 수 있도록 한다.
        // 이 우선순위 값은 큐가 1개 일 때도 지정해야한다.
        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    // 그래픽 카드의 적합성을 체크하는 함수
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        //      VkPhysicalDeviceProperties deviceProperties;
              //VkPhysicalDeviceFeatures deviceFeatures;

        //      // 디바이스 이름, 타입, 버전등의 정보는 아래 함수를 통해 가져올 수 있다.
              //vkGetPhysicalDeviceProperties(device, &deviceProperties);
        //      // 텍스처 압축, 64비트 float형, 멀티 뷰포트 렌더링 등의 기능 지원 여부는 아래 함수를 통해 알 수 있다.
        //      vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

              //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;


        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;

        if (extensionSupported) // extension support를 확인 한 후에 스왑체인 supprot 체크하는 것은 중요하다.
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };

    //이 함수를 작성함으로써 isDeviceSuitable 함수에서 해당 디바이스가 사용하고자 하는 커맨드를 처리할 수 있는 지 체크할 수 있다.
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        // 구조체를 위치시킬 큐 패밀리를 찾는 로직 작성
        // VkQueueFamilyProperties 구조체는 큐 패밀리의 디테일 정보를 담고 있음: 동작의 타입, 만들어질 수 있는 큐 개수....등
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // VK_QUEUE_GRAPHICS_BIT 를 지원하는 큐 패밀리를 최소 하나는 찾아야 한다.
        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = 1;
            }

            if (indices.isComplete())
            {
                break;
            }
            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,  // 메세지 심각도
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
            // pCallbackData 구조체는 아래와 같은 정보들을 담고 있다.
            // pMessage: 디버그 메세지 문자열
            // pObjects: 메세지와 관련된 Vulkan Object Handles 배열
            // objectCount: pObjects 배열의 길이
        }

        return VK_FALSE;
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
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

};

int main() 
{
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
