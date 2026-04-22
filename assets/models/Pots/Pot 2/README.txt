Ripped by Qw2, but many thanks to:
KillzXGaming and all the contributors to Switch-Toolbox, especially in relation to .mc / .txtg
Watertoon for their MC decompressor
Credit not necessary.

Notes:
 - Yellow normal maps (or maps with yellow spots) are maps where the blue channel is likely encoded as roughness/specular for compression purposes. You can use your program of choice to either set the blue channel to the maximum value for all pixels, or separate the RGB channels and use just the red and green as X and Y while leaving the Z vector as 1. For example, you can do this with Texture Remix (https://www.vg-resource.com/thread-26680.html) by assigning all the channels except blue (blue should be checked), and then exporting. Alternatively, you can also use the _fix image which has the blue channel maximized.
 
 - Textures with endings "_a", "_g", "_b", "_r" and the like are split textures as the primary texture in question stores data in separate channels. When extensions are combined (e.g. "_a_g_b") this means that those channels were identical for that image, i.e. in that case the alpha, green, and blue channel store the same data in that image.