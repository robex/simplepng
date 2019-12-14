## Easy to use png library

Compilation:

```make lib```

Example usage (rotate an image 90 degrees clockwise and save it to disk):

```
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