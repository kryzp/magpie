
internal BitmapImage
BitmapImageLoadFromFile(String8 path)
{
	BitmapImage image = {0};
	
	if(stbi_is_hdr((char *)path.str))
	{
		image.pixels = stbi_loadf((char *)path.str, &image.width, &image.height, &image.channels, 4);
		
		if(!image.pixels)
		{
			DebugLogCrash("Couldn't load Bitmap HDR: %s", path.str);
		}
		
		image.format = BitmapImageFormat_RGBAF;
	}
	else
	{
		image.pixels = stbi_load((char *)path.str, &image.width, &image.height, &image.channels, 4);
		
		if (!image.pixels)
		{
			DebugLogCrash("Couldn't load Bitmap LDR: %s", path.str);
		}
		
		image.format = BitmapImageFormat_RGBA8;
	}
	
	return image;
}

internal void
BitmapImageDestroy(BitmapImage *image)
{
	stbi_image_free(image->pixels);
	image->pixels = 0;
}

internal u64
GetBitmapImageMemorySize(BitmapImage *image)
{
	u64 unit = (image->format == BitmapImageFormat_RGBA8) ? sizeof(u8) : sizeof(f32);
	return image->width * image->height * 4 * unit;
}

internal Image
ImageFromBitmap(BitmapImage *bitmap)
{
	Assert(bitmap && "Bitmap image must not be null.");
	
	VkFormat format = VK_FORMAT_UNDEFINED;
	
	switch(bitmap->format)
	{
		case BitmapImageFormat_RGBA8:
		{
			format = VK_FORMAT_R8G8B8A8_UNORM;
		}
		break;
		
		case BitmapImageFormat_RGBAF:
		{
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		break;
	}
	
	u64 memory_size = GetBitmapImageMemorySize(bitmap);
	
	Image image = ImageAllocate(bitmap->width, bitmap->height, 1,
								format,
								VK_IMAGE_VIEW_TYPE_2D,
								VK_IMAGE_TILING_OPTIMAL,
								4,
								VK_SAMPLE_COUNT_1_BIT,
								false, false);
	
	GPUBuffer staging_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 memory_size);
	{
		GPUBufferWrite(&staging_buffer, bitmap->pixels, memory_size, 0);
		
		CommandBuffer cmd = BeginGraphicsInstantSubmit();
		{
			CmdTransitionImageLayout(&cmd, &image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			CmdCopyBufferToImage(&cmd, &staging_buffer, &image);
			CmdGenerateMipmaps(&cmd, &image);
		}
		EndGraphicsInstantSubmit(&cmd);
	}
	GraphicsWaitIdle();
	GPUBufferDestroy(&staging_buffer);
	
	return image;
}

internal Image
ImageFromPath(String8 path)
{
	BitmapImage bitmap = BitmapImageLoadFromFile(path);
	Image image = ImageFromBitmap(&bitmap);
	BitmapImageDestroy(&bitmap);
	return image;
}
