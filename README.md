## Easy to use PNG library

Compilation:

```make lib```

Example usage (rotate an image 90 degrees clockwise and save it to disk):

```
#include "png.h"

void rotate(char *filename)
{
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	png_rotate(&png);
	png_dump(&png, "rotated.png");
	png_close(&png);
}
```

**Warning**: PNG standard is not fully implemented. Support for working with things like black/white bitmaps and interlaced images will hopefully be added in the future.
