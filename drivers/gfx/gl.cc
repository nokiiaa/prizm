#include "driver.hh"
#include <sched/scheduler.hh>
#include <obj/process.hh>
#include <obj/gfx.hh>
#include "math.hh"
#include <debug.hh>

PzGraphicsObject *GfxAllocateObject(int obj_type, int size)
{
	PzGraphicsObject *graphics;

	if (!ObCreateUnnamedObject(
		ObGetObjectDirectory(PZ_OBJECT_GFX), (ObPointer *)&graphics, PZ_OBJECT_GFX, 0, false))
		return nullptr;

	graphics->ObjectType = obj_type;
	graphics->Data = PzHeapAllocate(size, HEAP_ZERO);

	if (!graphics->Data) {
		ObDereferenceObject(graphics);
		return nullptr;
	}

	return graphics;
}

void GfxDestroyObject(PzGraphicsObject *gfx)
{
	GfxRenderBuffer *render = (GfxRenderBuffer *)gfx->Data;
	GfxTextureBuffer *texture = (GfxTextureBuffer *)gfx->Data;
	GfxVertexBuffer *vertex = (GfxVertexBuffer *)gfx->Data;

	switch (gfx->ObjectType)
	{
	case GFX_RENDER_BUFFER:
		MmVirtualFreeMemory(render->PixelBufferDescriptor, render->PixelSize);
		if (render->ZBufferDescriptor)
			MmVirtualFreeMemory(render->ZBufferDescriptor, render->ZSize);
		break;

	case GFX_TEXTURE_BUFFER:
		MmVirtualFreeMemory(texture->MemoryDescriptor, texture->Size);
		break;

	case GFX_VERTEX_BUFFER:
		MmVirtualFreeMemory(vertex->MemoryDescriptor, vertex->Size);
		break;
	}

	PzHeapFree(gfx->Data);
}

template<class T = void*> bool GfxFindObject(int type, GfxHandle id, PzGraphicsObject *&obj_ptr, T &ptr)
{
	PzGraphicsObject *obj;

	if (!ObReferenceObjectByHandle(PZ_OBJECT_GFX, nullptr, id, (ObPointer *)&obj))
		return false;

	if (obj->ObjectType != type) {
		ObDereferenceObject(obj);
		return false;
	}

	obj_ptr = obj;
	ptr = (T)obj->Data;
	return true;
}

PzStatus GfxGenerateBuffers(int type, int count, GfxHandle *handles)
{
    int size = 0;
    switch (type) {
    case GFX_RENDER_BUFFER:
        size = sizeof(GfxRenderBuffer);
        break;

    case GFX_TEXTURE_BUFFER:
        size = sizeof(GfxTextureBuffer);
        break;

    case GFX_VERTEX_BUFFER:
        size = sizeof(GfxVertexBuffer);
        break;

    default:
        return STATUS_INVALID_ARGUMENT;
    }

	for (int i = 0; i < count; i++) {
		PzHandle handle;
		PzGraphicsObject *graphics = GfxAllocateObject(type, size);

		if (!graphics)
			return STATUS_ALLOCATION_FAILED;

		if (!ObCreateHandle(nullptr, 0, &handle, graphics)) {
			ObDereferenceObject(graphics);
			return STATUS_FAILED;
		}

		ObDereferenceObject(graphics);
		handles[i] = handle;
	}

    return STATUS_SUCCESS;
}

PzStatus GfxTextureData(GfxHandle handle,
    int width, int height,
    int pixel_format, int flags, void *data)
{
    GfxTextureBuffer *texture;
	PzGraphicsObject *graphics;

    if (!GfxFindObject(GFX_TEXTURE_BUFFER, handle, graphics, texture))
        return STATUS_INVALID_HANDLE;

	u32 data_size = width * height * sizeof(u32);

	void *copy = MmVirtualAllocateMemory(nullptr, data_size, PAGE_READWRITE, nullptr);

	if (!copy) {
		ObDereferenceObject(graphics);
		return STATUS_ALLOCATION_FAILED;
	}

	MemCopy(copy, data, data_size);

	if (texture->HasData)
		MmVirtualFreeMemory(texture->MemoryDescriptor, texture->Size);

    texture->Width = width;
    texture->Height = height;
    texture->PixelFormat = pixel_format;
	texture->Size = data_size;
    texture->Flags = flags;
    texture->MemoryDescriptor = copy;
	texture->HasData = true;

	ObDereferenceObject(graphics);
    return STATUS_SUCCESS;
}

PzStatus GfxRenderBufferData(GfxHandle handle,
	int width, int height,
	int pixel_format)
{
	GfxRenderBuffer *render;
	PzGraphicsObject *graphics;

	if (!GfxFindObject(GFX_RENDER_BUFFER, handle, graphics, render))
		return STATUS_INVALID_HANDLE;

	u32 pixel_size = width * height * sizeof(u32);
	u32 zbuf_size = width * height * sizeof(float);

	void *pixel_new = MmVirtualAllocateMemory(nullptr, pixel_size, PAGE_READWRITE, nullptr);
	void *zbuf_new = MmVirtualAllocateMemory(nullptr, zbuf_size, PAGE_READWRITE, nullptr);

	if (!pixel_new) {
		ObDereferenceObject(graphics);
		return STATUS_ALLOCATION_FAILED;
	}

	if (!zbuf_new) {
		MmVirtualFreeMemory(pixel_new, pixel_size);
		ObDereferenceObject(graphics);
		return STATUS_ALLOCATION_FAILED;
	}

	if (render->HasData) {
		MmVirtualFreeMemory(render->PixelBufferDescriptor, render->PixelSize);
		MmVirtualFreeMemory(render->ZBufferDescriptor, render->ZSize);
	}

	render->Width = width;
	render->Height = height;
	render->PixelFormat = pixel_format;

	render->PixelSize = pixel_size;
	render->PixelBufferDescriptor = pixel_new;

	render->ZSize = zbuf_size;
	render->ZBufferDescriptor = zbuf_new;

	render->HasData = true;

	ObDereferenceObject(graphics);
	return STATUS_SUCCESS;
}

PzStatus GfxTextureFlags(GfxHandle handle, u32 flags)
{
    GfxTextureBuffer *texture;
	PzGraphicsObject *graphics;

    if (!GfxFindObject(GFX_TEXTURE_BUFFER, handle, graphics, texture))
        return STATUS_INVALID_HANDLE;

    texture->Flags = flags;

	ObDereferenceObject(graphics);
    return STATUS_SUCCESS;
}

PzStatus GfxVertexBufferData(GfxHandle handle, void *data, u32 size)
{
    GfxVertexBuffer *vertex;
	PzGraphicsObject *graphics;

    if (!GfxFindObject(GFX_VERTEX_BUFFER, handle, graphics, vertex))
        return STATUS_INVALID_HANDLE;

	void *copy = MmVirtualAllocateMemory(nullptr, size, PAGE_READWRITE, nullptr);

	if (!copy) {
		ObDereferenceObject(graphics);
		return STATUS_ALLOCATION_FAILED;
	}

	MemCopy(copy, data, size);

	if (vertex->HasData)
		MmVirtualFreeMemory(vertex->MemoryDescriptor, vertex->Size);

    vertex->MemoryDescriptor = copy;
    vertex->Size = size;
	vertex->HasData = true;

	ObDereferenceObject(graphics);
    return STATUS_SUCCESS;
}

PzStatus GfxClear(GfxHandle render_handle, u32 flags)
{
    GfxRenderBuffer *render;
	PzGraphicsObject *graphics;

    if (!GfxFindObject(GFX_RENDER_BUFFER, render_handle, graphics, render))
        return STATUS_INVALID_HANDLE;

	if (!render->HasData) {
		ObDereferenceObject(graphics);
		return STATUS_NO_DATA;
	}

    if (flags & GFX_CLEAR_COLOR)
		MemSet(render->PixelBufferDescriptor, 0, render->PixelSize);

	if (flags & GFX_CLEAR_DEPTH)
		MemSet(render->ZBufferDescriptor, 0, render->ZSize);

	ObDereferenceObject(graphics);
	return STATUS_SUCCESS;
}

PzStatus GfxClearColor(GfxHandle render_handle, u32 color)
{
	GfxRenderBuffer *render;
	PzGraphicsObject *graphics;

	if (!GfxFindObject(GFX_RENDER_BUFFER, render_handle, graphics, render))
		return STATUS_INVALID_HANDLE;

	if (!render->HasData) {
		ObDereferenceObject(graphics);
		return STATUS_NO_DATA;
	}

	for (int i = 0; i < render->PixelSize / 4; i++)
		((u32 *)render->PixelBufferDescriptor)[i] = color;

	ObDereferenceObject(graphics);
	return STATUS_SUCCESS;
}

PzStatus GfxUploadToDisplay(GfxHandle render_buffer)
{
    GfxRenderBuffer *render;
	PzGraphicsObject *graphics;

    if (!GfxFindObject(GFX_RENDER_BUFFER, render_buffer, graphics, render))
        return STATUS_INVALID_HANDLE;

	if (!render->HasData) {
		ObDereferenceObject(graphics);
		return STATUS_NO_DATA;
	}

	if (VbeData->Width != render->Width || VbeData->Height != render->Height) {
		ObDereferenceObject(graphics);
		return STATUS_INVALID_ARGUMENT;
	}

    MemCopy(VbeFramebuffer, render->PixelBufferDescriptor, render->PixelSize);
	ObDereferenceObject(graphics);
    return STATUS_SUCCESS;
}

PzStatus GfxBitBlit(
    GfxHandle dest_buffer, int dx, int dy, int dw, int dh,
    GfxHandle  src_buffer, int sx, int sy)
{
	PzGraphicsObject *dest_graphics, *src_graphics;

	if (!ObReferenceObjectByHandle(PZ_OBJECT_GFX, nullptr, dest_buffer, (ObPointer *)&dest_graphics))
		return STATUS_INVALID_HANDLE;

	if (!ObReferenceObjectByHandle(PZ_OBJECT_GFX, nullptr, src_buffer, (ObPointer *)&src_graphics)) {
		ObDereferenceObject(dest_graphics);
		return STATUS_INVALID_HANDLE;
	}

	if (dest_graphics->ObjectType != GFX_RENDER_BUFFER &&
		dest_graphics->ObjectType != GFX_TEXTURE_BUFFER ||
		src_graphics->ObjectType != GFX_RENDER_BUFFER &&
		src_graphics->ObjectType != GFX_TEXTURE_BUFFER) {
		ObDereferenceObject(dest_graphics);
		ObDereferenceObject(src_graphics);
		return STATUS_INVALID_ARGUMENT;
	}

	GfxRenderBuffer *render_buffer;
	GfxTextureBuffer *texture_buffer;

	int dest_width, dest_height;
	int src_width, src_height;
	void *dest_data, *src_data;

	switch (dest_graphics->ObjectType) {
	case GFX_RENDER_BUFFER:
		render_buffer = (GfxRenderBuffer*)dest_graphics->Data;

		if (!render_buffer->HasData)
			goto fail_no_data;

		dest_data = render_buffer->PixelBufferDescriptor;
		dest_width = render_buffer->Width;
		dest_height = render_buffer->Height;
		break;

	case GFX_TEXTURE_BUFFER:
		texture_buffer = (GfxTextureBuffer *)dest_graphics->Data;

		if (!texture_buffer->HasData)
			goto fail_no_data;

		dest_data = texture_buffer->MemoryDescriptor;
		dest_width = texture_buffer->Width;
		dest_height = texture_buffer->Height;
		break;
	}

	switch (src_graphics->ObjectType) {
	case GFX_RENDER_BUFFER:
		render_buffer = (GfxRenderBuffer *)src_graphics->Data;

		if (!render_buffer->HasData)
			goto fail_no_data;

		src_data = render_buffer->PixelBufferDescriptor;
		src_width = render_buffer->Width;
		src_height = render_buffer->Height;
		break;

	case GFX_TEXTURE_BUFFER:
		texture_buffer = (GfxTextureBuffer *)src_graphics->Data;

		if (!texture_buffer->HasData)
			goto fail_no_data;

		src_data = texture_buffer->MemoryDescriptor;
		src_width = texture_buffer->Width;
		src_height = texture_buffer->Height;
		break;
	}

	if (sx < 0) { dx -= sx; dw += sx; sx = 0; }
	if (sy < 0) { dy -= sy; dy += sy; sy = 0; }
	if (dx < 0) { dw += dx; dx = 0; }
	if (dy < 0) { dh += dy; dy = 0; }

	dw = Min(dw, src_width - sx);
	dw = Min(dw, dest_width - dx);
	dh = Min(dh, src_height - sy);
	dh = Min(dh, dest_height - dy);

	if (dw > 0 && dh > 0) {
		for (int y = 0; y < dh; y++) {
			for (int x = 0; x < dw; x++) {
				u32& pixel = ((u32 *)dest_data)[dest_width * (y + dy) + x + dx];
				pixel = Color(pixel).Alphablend(
					((u32 *)src_data)[src_width * (y + sy) + x + sx]);
			}
		}
	}

	ObDereferenceObject(dest_graphics);
	ObDereferenceObject(src_graphics);

    return STATUS_SUCCESS;

fail_no_data:
	ObDereferenceObject(dest_graphics);
	ObDereferenceObject(src_graphics);

	return STATUS_NO_DATA;
}

void Triangle(GfxRenderBuffer *render, Vec3 p[3],
	GfxTextureBuffer *texture, Vec2 uvs[3],
	Color colors[3], float fov_tan, float near_plane)
{
	// Incredible clipping algorithm :d
	unsigned outside = 0, mask = 0;

	if (p[0].Z < near_plane) {
		outside++;
		mask |= 4;
	}

	if (p[1].Z < near_plane) {
		outside++;
		mask |= 2;
	}

	if (p[2].Z < near_plane) {
		outside++;
		mask |= 1;
	}

	// One vertex outside the view
	if (outside == 1) {
		int in1, in2, out;

		switch (mask) {
		case 0b100: in1 = 1, in2 = 2, out = 0; break;
		case 0b010: in1 = 0, in2 = 2, out = 1; break;
		case 0b001: in1 = 0, in2 = 1, out = 2; break;
		}

		float t1 = (near_plane - p[in1].Z) / (p[out].Z - p[in1].Z);
		float t2 = (near_plane - p[in2].Z) / (p[out].Z - p[in2].Z);
		Vec3 intersect1 = Math::Lerp(p[in1], p[out], t1);
		Vec3 intersect2 = Math::Lerp(p[in2], p[out], t2);
		Vec2 intersect1_uv = Math::Lerp(uvs[in1], uvs[out], t1);
		Vec2 intersect2_uv = Math::Lerp(uvs[in2], uvs[out], t2);
		Vec3 verts1[] = { intersect1, intersect2, p[in1] };
		Vec3 verts2[] = { intersect2, p[in1], p[in2] };
		Vec2 uvs1[] = { intersect1_uv, intersect2_uv, uvs[in1] };
		Vec2 uvs2[] = { intersect2_uv, uvs[in1], uvs[in2] };

		Triangle(render, verts1, texture, uvs1, colors, fov_tan, near_plane);
		Triangle(render, verts2, texture, uvs2, colors, fov_tan, near_plane);
		return;
	}
	// Two vertices outside the view
	else if (outside == 2) {
		int in, o1, o2;

		switch (mask) {
		case 0b101: in = 1, o1 = 0, o2 = 2; break;
		case 0b110: in = 2, o1 = 0, o2 = 1; break;
		case 0b011: in = 0, o1 = 1, o2 = 2; break;
		}

		float t1 = (near_plane - p[in].Z) / (p[o1].Z - p[in].Z);
		float t2 = (near_plane - p[in].Z) / (p[o2].Z - p[in].Z);
		p[o1] = Math::Lerp(p[in], p[o1], t1);
		p[o2] = Math::Lerp(p[in], p[o2], t2);
		uvs[o1] = Math::Lerp(uvs[in], uvs[o1], t1);
		uvs[o2] = Math::Lerp(uvs[in], uvs[o2], t2);

		Triangle(render, p, texture, uvs, colors, fov_tan, near_plane);
		return;
	}
	else if (outside == 3)
		return;

	Vec2 a, b, c;
	float center_x = render->Width / 2,
		center_y = render->Height / 2;
	float depth_a, depth_b, depth_c;

	// Perspective-project points
	depth_a = 1.f / (fov_tan * p[0].Z);
	depth_b = 1.f / (fov_tan * p[1].Z);
	depth_c = 1.f / (fov_tan * p[2].Z);
	a.X = int(center_x + depth_a * p[0].X * center_x);
	a.Y = int(center_y - depth_a * p[0].Y * center_x);
	b.X = int(center_x + depth_b * p[1].X * center_x);
	b.Y = int(center_y - depth_b * p[1].Y * center_x);
	c.X = int(center_x + depth_c * p[2].X * center_x);
	c.Y = int(center_y - depth_c * p[2].Y * center_x);

	// Original A, B and C points of triangle before being sorted.
	Vec2 org_a = a, org_b = b, org_c = c;

	// Sort the points by Y in ascending order
	if (a.Y > b.Y) Swap(a, b);
	if (a.Y > c.Y) Swap(a, c);
	if (b.Y > c.Y) Swap(b, c);

	// Sort Y-equal points by X in ascending order
	if (b.Y == c.Y && b.X > c.X) Swap(b, c);
	if (a.Y == b.Y && a.X > b.X) Swap(a, b);
	if (a.Y == c.Y && a.X > c.X) Swap(a, c);

	// Cull 100% invisible cases
	if (c.Y < 0 ||
		a.Y > render->Height ||
		a.X >= render->Width && b.X >= render->Width && c.X >= render->Width ||
		a.X < 0 && b.X < 0 && c.X < 0)
		return;

	Vec2 a1, a2, b1, b2;

	// Starting lines
	a1 = a;
	a2 = c;

	if (a.Y == b.Y) {
		b1 = b;
		b2 = c;
	}
	else {
		b1 = a;
		b2 = b;
	}

	Vec2 v0 = org_b - org_a,
	     v1 = org_c - org_a;
	int flat = depth_a == depth_b && depth_b == depth_c;
	float depth_factor = 1.f / depth_a;
	float bary_inverse = 1.f / (v0.X * v1.Y - v1.X * v0.Y);
	float x1 = a1.X, x2 = b1.X;
	float step1 = (float)(a2.X - a1.X) / (a2.Y - a1.Y);
	float step2 = (float)(b2.X - b1.X) / (b2.Y - b1.Y);
	int y1 = a.Y;

	if (y1 < 0) {
		x1 = a1.X + step1 * -y1;
		x2 = b1.X + step2 * -y1;
		y1 = 0;
	}

	float error1 = 0, error2 = 0;
	
	// Calculate and fill region (a1; a2; b1; b2) line by line
	for (int y = y1, y2 = c.Y; y < y2; y++) {
		if (y >= render->Height)
			break;

		// If region ended, switch to next vertex
		int steps;
		if (y >= a2.Y) {
			steps = y - a2.Y;
			a1 = a2;
			a2 = c;
			step1 = (float)(a2.X - a1.X) / (a2.Y - a1.Y);
			error1 = 0;
			x1 = a1.X + step1 * steps;
		}

		if (y >= b2.Y) {
			steps = y - b2.Y;
			b1 = b2;
			b2 = c;
			step2 = (float)(b2.X - b1.X) / (b2.Y - b1.Y);
			error2 = 0;
			x2 = b1.X + step2 * steps;
		}

		// Calculate x coordinates of intersections with the scanline
		int ix1 = (int)Math::Round(x1);
		int ix2 = (int)Math::Round(x2);
		if (ix1 > ix2)     Swap(ix1, ix2);
		if (ix1 < 0)       ix1 = 0;
		if (ix2 > render->Width) ix2 = render->Width;
		float inv = depth_factor;

		for (int x = ix1; ix2 > x; x++) {
			// Derive barycentric coordinates of point (x; y)
			Vec2 v2(x - org_a.X, y - org_a.Y);
			float inc_bary_b = (v2.X * v1.Y - v1.X * v2.Y) * bary_inverse;
			float inc_bary_c = (v0.X * v2.Y - v2.X * v0.Y) * bary_inverse;
			float inc_bary_a = 1.f - inc_bary_b - inc_bary_c;

			// Derive perspective-corrected barycentric coordinates
			float c1 = inc_bary_a * depth_a, c2 = inc_bary_b * depth_b, c3 = inc_bary_c * depth_c;
			float z_val = c1 + c2 + c3;

			if (z_val > ((float*)render->ZBufferDescriptor)[render->Width * y + x]) {
				// Avoid floating-point division when polygon is flat and dividend is constant
				// (sum of barycentric coordinates is always 1, so
				// 1.f / (uncBaryA * depth + uncBaryB * depth + uncBaryC * depth) will
				// always equal depth)
				if (!flat)
					inv = 1.f / z_val;

				float bary_a = Math::Clamp(c1 * inv, 0.f, 1.f),
					bary_b = Math::Clamp(c2 * inv, 0.f, 1.f),
					bary_c = Math::Clamp(c3 * inv, 0.f, 1.f);
				u32 color;

				if (texture) {
					float uf = Math::Clamp(bary_a * uvs[0].X + bary_b * uvs[1].X + bary_c * uvs[2].X, 0.f, 1.f);
					float vf = Math::Clamp(bary_a * uvs[0].Y + bary_b * uvs[1].Y + bary_c * uvs[2].Y, 0.f, 1.f);
					color = ((u32 *)texture->MemoryDescriptor)[
						texture->Width * (int)(texture->Height * vf) + (int)(texture->Width * uf)];
				}
				else
					color = colors[0] * bary_a + colors[1] * bary_b + colors[2] * bary_c;
				u32 &pixel = ((u32 *)render->PixelBufferDescriptor)[y * render->Width + x];
				pixel = Color(pixel).Alphablend(color);
				((float *)render->ZBufferDescriptor)[y * render->Width + x] = z_val;
			}
		}

		x1 += step1;
		x2 += step2;
	}
}

void DrawTriangleFromVertices(
	GfxRenderBuffer *render, GfxTextureBuffer *texture,
	bool floating, bool uv, u8 **tri_vertices)
{
	Vec3 points[3];
	Vec2 uvs[3];
	Color colors[3] = { 0, 0, 0 };

	for (int i = 0; i < 3; i++) {
		GfxVertexFormat *buf_vertex = (GfxVertexFormat *)tri_vertices[i];

		if (!floating) {
			points[i] = {
				buf_vertex->IntCoords[0] / (float)render->Width,
				buf_vertex->IntCoords[1] / (float)render->Height,
				(float)buf_vertex->IntCoords[2]
			};
		}
		else {
			points[i] = {
				buf_vertex->FloatCoords[0],
				buf_vertex->FloatCoords[1],
				buf_vertex->FloatCoords[2]
			};
		}

		if (uv) {
			if (floating)
				uvs[i] = { buf_vertex->FloatU, buf_vertex->FloatV };
			else
				uvs[i] = {
					buf_vertex->IntU / (float)texture->Width,
					buf_vertex->IntV / (float)texture->Height
				};
		}
		else {
			colors[i] = Color(
				buf_vertex->R, buf_vertex->G,
				buf_vertex->B, buf_vertex->A
			);
		}
	}

	Triangle(render, points, texture, uvs, colors, 1, 0.03f);
}

PzStatus GfxDrawRectangle(
	GfxHandle render_handle, GfxHandle texture_handle,
	int x, int y, int width, int height, u32 color, bool int_uvs, const void *uvs)
{
	PzGraphicsObject *graphics1, *graphics2 = nullptr;
	GfxRenderBuffer *render;
	GfxTextureBuffer *texture = nullptr;

	if (!GfxFindObject(GFX_RENDER_BUFFER, render_handle, graphics1, render))
		return STATUS_INVALID_HANDLE;

	if (texture_handle && !GfxFindObject(GFX_TEXTURE_BUFFER, texture_handle, graphics2, texture)) {
		ObDereferenceObject(graphics1);
		return STATUS_INVALID_HANDLE;
	}

	if (!render->HasData ||
		texture && !texture->HasData) {
		ObDereferenceObject(graphics1);

		if (graphics2)
			ObDereferenceObject(graphics2);

		return STATUS_NO_DATA;
	}

	int rx = x, ry = y, rw = width, rh = height;
	if (x < 0) { width  += x; x = 0; }
	if (y < 0) { height += y; y = 0; }
	int dx = x - rx, dy = y - ry;

	float scale_w = 1.f / rw;
	float scale_h = 1.f / rh;
	int *uvs_int = (int *)uvs;
	float *uvs_float = (float *)uvs;
	width  = Min(width,  render->Width  - x);
	height = Min(height, render->Height - y);

	if (width >= 0 && height >= 0) {
		if (texture) {
			if (int_uvs) {
				for (int py = 0; py < height; py++) {
					for (int px = 0; px < width; px++) {
						float tx = (px + dx) * scale_w;
						float ty = (py + dy) * scale_h;

						int u =
							unsigned(Math::BiLerp(
								uvs_int[0], uvs_int[2], uvs_int[4], uvs_int[6], tx, ty))
							% texture->Width;

						int v =
							unsigned(Math::BiLerp(
								uvs_int[1], uvs_int[3], uvs_int[5], uvs_int[7], tx, ty))
							% texture->Height;

						u32 &pixel = ((u32 *)render->PixelBufferDescriptor)[render->Width * (py + y) + px + x];
						pixel = Color(pixel).Alphablend(
							((u32 *)texture->MemoryDescriptor)[v * texture->Width + u]);
					}
				}
			}
			else {
				for (int py = 0; py < height; py++) {
					for (int px = 0; px < width; px++) {
						float tx = (px + dx) * scale_w;
						float ty = (py + dy) * scale_h;

						int u =
							unsigned(Math::BiLerp(
								uvs_float[0], uvs_float[2], uvs_float[4], uvs_float[6], tx, ty)
								* texture->Width)
							% texture->Width;

						int v =
							unsigned(Math::BiLerp(
								uvs_float[1], uvs_float[3], uvs_float[5], uvs_float[7], tx, ty)
								* texture->Height)
							% texture->Height;

						u32 &pixel = ((u32 *)render->PixelBufferDescriptor)[render->Width * (py + y) + px + x];
						pixel = Color(pixel).Alphablend(
							((u32 *)texture->MemoryDescriptor)[v * texture->Width + u]);
					}
				}
			}
		}
		else {
			for (int py = 0; py < height; py++) {
				for (int px = 0; px < width; px++) {
					u32 &pixel = ((u32 *)render->PixelBufferDescriptor)[render->Width * (py + y) + px + x];
					pixel = Color(pixel).Alphablend(color);
				}
			}
		}
	}

	ObDereferenceObject(graphics1);
	if (graphics2)
		ObDereferenceObject(graphics2);

	return STATUS_SUCCESS;
}

PzStatus GfxDrawTriangles(
	GfxHandle render_handle, GfxHandle vbo_handle,
	GfxHandle texture_handle, int data_format,
	int start_index, int vertex_count)
{
	GfxRenderBuffer *render;
	GfxVertexBuffer *vbo;
	GfxTextureBuffer *texture = nullptr;
	PzGraphicsObject *graphics1, *graphics2, *graphics3 = nullptr;

	if (!GfxFindObject(GFX_RENDER_BUFFER, render_handle, graphics1, render))
		return STATUS_INVALID_HANDLE;

	if (!GfxFindObject(GFX_VERTEX_BUFFER, vbo_handle, graphics2, vbo)) {
		ObDereferenceObject(graphics1);
		return STATUS_INVALID_HANDLE;
	}

	if (texture_handle && !GfxFindObject(GFX_TEXTURE_BUFFER, texture_handle, graphics3, texture)) {
		ObDereferenceObject(graphics1);
		ObDereferenceObject(graphics2);
		return STATUS_INVALID_HANDLE;
	}

	if (!render->HasData ||
		!vbo->HasData ||
		texture && !texture->HasData) {
		ObDereferenceObject(graphics1);
		ObDereferenceObject(graphics2);

		if (graphics3)
			ObDereferenceObject(graphics3);

		return STATUS_NO_DATA;
	}

    bool floating = (data_format & GFX_FORMAT_FLOAT) == GFX_FORMAT_FLOAT;
    bool uv = (data_format & GFX_FORMAT_UV) == GFX_FORMAT_UV;
    int type = data_format & ~(GFX_FORMAT_UV | GFX_FORMAT_FLOAT);
    int vertex_size = sizeof(float) * 3 + (uv ? 2 * sizeof(float) : 4);
    u8 *buf = (u8 *)vbo->MemoryDescriptor + start_index * vertex_size;
    u8 *tri_vertices[3];

    for (int i = 0; i < vertex_count; i++) {
        switch (type) {
        case GFX_TRI_STRIP:
            if (i >= 3) {
                tri_vertices[0] = tri_vertices[1];
                tri_vertices[1] = tri_vertices[2];
            }
            tri_vertices[Min(i, 2)] = buf;
            if (i >= 2)
				DrawTriangleFromVertices(render, texture, floating, uv, tri_vertices);
            break;

        case GFX_TRI_LIST:
            tri_vertices[i % 3] = buf;
            if (i % 3 == 2)
				DrawTriangleFromVertices(render, texture, floating, uv, tri_vertices);
            break;
        }

        buf += vertex_size;
    }

	ObDereferenceObject(graphics1);
	ObDereferenceObject(graphics2);

	if (graphics3)
		ObDereferenceObject(graphics3);

	return STATUS_SUCCESS;
}