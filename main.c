#include "vkRenderer.h"

int main(void){

	vk_renderer_create();

	while(vk_drawFrame());

	vk_renderer_destroy();

	return 0;
}
