VULKAN_SDK_PATH = /home/joshua/Software/VulkanSDK/1.0.65.0/x86_64
STB_INCLUDE_PATH = /home/joshua/Software/stb
TINYOBJ_INCLUDE_PATH = /home/joshua/Software/tinyobjloader

CFLAGS = -std=c++11 -g -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH) -I$(TINYOBJ_INCLUDE_PATH) -O3
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanApplication: main.cpp
	g++ $(CFLAGS) -o VulkanApplication Vertex.cpp DeviceManager.cpp SwapchainManager.cpp UniformManager.cpp Utils.cpp main.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanApplication
	LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d ./VulkanApplication

clean:
	rm -f VulkanApplication
