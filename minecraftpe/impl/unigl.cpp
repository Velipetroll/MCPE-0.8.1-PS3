#include <unigl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmath>

extern "C" {
	#include <rsx/rsx.h>
	#include <rsx/nv40.h>
	#include <rsx/commands.h>
}

#ifndef GCM_INVALIDATE_TEXTURE
#define GCM_INVALIDATE_TEXTURE 1
#endif

typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef ptrdiff_t GLsizeiptr;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef float GLclampf;
typedef void GLvoid;

extern gcmContextData *rsx_ctx;

extern "C" {
	extern const uint8_t vertex_vcg_vpo[];
	extern const uint8_t fragment_fcg_fpo[];
}

struct Mat4 { float m[16]; };

static u32 global_rsx_ref = 1;

static inline void wait_rsx_idle() {
	rsxFinish(rsx_ctx, global_rsx_ref++);
}

static int dbg_frames = 0;
static int dbg_draws = 0;
static int dbg_verts = 0;
static int dbg_max_idx = 0;
static bool dbg_nan = false;
static int dbg_flushes = 0;

static bool is_matrix_corrupt(const Mat4& m) {
	for(int i=0; i<16; i++) {
		if(std::isnan(m.m[i]) || std::isinf(m.m[i])) return true;
	}
	return false;
}

static inline void cpu_cache_flush(void* ptr, uint32_t size) {
	uintptr_t start = (uintptr_t)ptr & ~127;
	uintptr_t end = ((uintptr_t)ptr + size + 127) & ~127;
	for (uintptr_t i = start; i < end; i += 128) {
		__asm__ __volatile__("dcbf 0, %0" :: "r"(i));
	}
	__asm__ __volatile__("sync");
}

static Mat4 mat_identity() {
	Mat4 res; memset(&res, 0, sizeof(res)); res.m[0] = res.m[5] = res.m[10] = res.m[15] = 1.0f;
	return res;
}

static Mat4 mat_mul(const Mat4& a, const Mat4& b) {
	Mat4 res;
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			res.m[c*4 + r] = a.m[0*4 + r] * b.m[c*4 + 0] + a.m[1*4 + r] * b.m[c*4 + 1] + a.m[2*4 + r] * b.m[c*4 + 2] + a.m[3*4 + r] * b.m[c*4 + 3];
		}
	}
	return res;
}

static Mat4 mat_transpose(const Mat4& in) {
	Mat4 out;
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) out.m[r * 4 + c] = in.m[c * 4 + r];
	}
	return out;
}

static std::vector<Mat4> stack_modelview = { mat_identity() };
static std::vector<Mat4> stack_projection = { mat_identity() };
static GLenum current_matrix_mode = 0x1700; // GL_MODELVIEW

static const void* vtx_ptr = NULL;
static GLsizei vtx_stride = 0;
static unsigned int type_vtx = 0x1406;
static int size_vtx = 3;

static const void* tex_coord_ptr = NULL;
static GLsizei tex_coord_stride = 0;
static unsigned int type_tex_coord = 0x1406;
static int size_tex_coord = 2;

static const void* col_ptr = NULL;
static GLsizei col_stride = 0;
static unsigned int type_color = 0;
static int size_color = 4;

static const void* norm_ptr = NULL;
static GLsizei norm_stride = 0;
static unsigned int type_norm = 0x1406;

static bool vtx_enabled = false, col_enabled = false, tex_enabled = false;
static bool g_rsx_initialized = false;
static rsxVertexProgram *g_vpo = NULL;
static rsxFragmentProgram *g_fpo = NULL;
static void *g_vp_ucode = NULL, *g_fp_ucode = NULL, *g_fp_rsx_mem = NULL;
static u32 g_fp_offset = 0;

static const rsxProgramConst* g_param_mvp = NULL;
static const rsxProgramConst* g_param_u_color = NULL;
static const rsxProgramConst* g_param_use_vtx_color = NULL;
static const rsxProgramConst* g_param_u_flags = NULL;

static bool g_blend_enabled = false, g_depth_test_enabled = true, g_depth_write_enabled = true;
static bool g_alpha_test_enabled = false, g_cull_face_enabled = true, g_texture_2d_enabled = true;
static bool g_stencil_test_enabled = false;
static bool g_poly_offset_fill = false;
static float g_alpha_ref = 0.1f;
static uint32_t g_color_mask = GCM_COLOR_MASK_R | GCM_COLOR_MASK_G | GCM_COLOR_MASK_B | GCM_COLOR_MASK_A;
static uint32_t g_stencil_func = GCM_ALWAYS;
static int32_t g_stencil_ref = 0;
static uint32_t g_stencil_mask = 0xFF;
static uint32_t g_stencil_fail = GCM_KEEP;
static uint32_t g_stencil_zfail = GCM_KEEP;
static uint32_t g_stencil_zpass = GCM_KEEP;

// Guardamos la funcion original de profundidad
static uint32_t g_depth_func = GCM_LEQUAL;

static bool g_lighting_enabled = false;
static bool norm_enabled = false;
static GLuint g_norm_vbo = 0;

static int g_vp_x = 0, g_vp_y = 0, g_vp_w = 4096, g_vp_h = 4096;
static float g_z_near = 0.0f, g_z_far = 1.0f;
static int g_screen_height = 480;

static bool g_scissor_enabled = false;
static int g_scissor_x = 0, g_scissor_y = 0, g_scissor_w = 4096, g_scissor_h = 4096;

static uint32_t clear_color_packed = 0xFF000000;
static uint8_t cur_r = 255, cur_g = 255, cur_b = 255, cur_a = 255;

struct RSXBuffer { void* cpu_mem = NULL; u32 size = 0; };
static std::map<GLuint, RSXBuffer> g_vbo_map;
static GLuint g_bound_vbo = 0, g_bound_ibo = 0;
static GLuint g_vtx_vbo = 0, g_col_vbo = 0, g_tex_vbo = 0;

struct RSXTexture { gcmTexture tex; void* rsx_mem = NULL; u32 size = 0; u32 offset = 0; int w = 0; int h = 0; };
static std::map<GLuint, RSXTexture> g_tex_map;
static GLuint g_bound_tex = 0, g_next_tex_id = 1;

#define DYNAMIC_RSX_VTX_SIZE (16 * 1024 * 1024)
static void* g_dynamic_vtx_mem = NULL;
static u32 g_dynamic_vtx_offset = 0;
static u32 g_dynamic_vtx_curr = 0;

static void copy_pixels(uint8_t* dst_bytes, const uint8_t* src, int xoffset, int yoffset, int width, int height, u32 pitch, int tex_w, int tex_h, unsigned int format, unsigned int type) {
	for (int r = 0; r < height; r++) {
		int dst_y = yoffset + r;
		if (dst_y >= tex_h) break;
		for(int c = 0; c < width; c++) {
			int dst_x = xoffset + c;
			if (dst_x >= tex_w) break;

			int idx = (r * width + c) * 4;

			uint8_t R = src[idx + 0];
			uint8_t G = src[idx + 1];
			uint8_t B = src[idx + 2];
			uint8_t A = src[idx + 3];

			if (format == 0x1907) { // GL_RGB
				A = 255;
			}

			dst_bytes[dst_y * pitch + dst_x*4 + 0] = A;
			dst_bytes[dst_y * pitch + dst_x*4 + 1] = R;
			dst_bytes[dst_y * pitch + dst_x*4 + 2] = G;
			dst_bytes[dst_y * pitch + dst_x*4 + 3] = B;
		}
	}
}

static void impl_glDeleteBuffers(GLsizei n, const GLuint *b) {
	if (!b) return;
	for (int i = 0; i < n; i++) {
		auto it = g_vbo_map.find(b[i]);
		if (it != g_vbo_map.end()) {
			if (it->second.cpu_mem) free(it->second.cpu_mem);
			g_vbo_map.erase(it);
		}
		if (g_bound_vbo == b[i]) g_bound_vbo = 0;
		if (g_bound_ibo == b[i]) g_bound_ibo = 0;
		if (g_vtx_vbo == b[i]) g_vtx_vbo = 0;
		if (g_col_vbo == b[i]) g_col_vbo = 0;
		if (g_tex_vbo == b[i]) g_tex_vbo = 0;
		if (g_norm_vbo == b[i]) g_norm_vbo = 0;
	}
}

static void impl_glGenBuffers(GLsizei n, GLuint* buffers) {
	static GLuint next_vbo_id = 1;
	for (int i = 0; i < n; i++) buffers[i] = next_vbo_id++;
}

static void impl_glBindBuffer(GLenum target, GLuint buffer) {
	if (target == 34963 || target == 0x8893) g_bound_ibo = buffer;
	else g_bound_vbo = buffer;
}

static void impl_glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
	GLuint bound = (target == 34963 || target == 0x8893) ? g_bound_ibo : g_bound_vbo;
	if (bound != 0 && size > 0) {
		RSXBuffer& buf = g_vbo_map[bound];
		if (!buf.cpu_mem || buf.size < (u32)size) {
			if (buf.cpu_mem) free(buf.cpu_mem);
			buf.cpu_mem = malloc(size);
			buf.size = size;
		}
		if (data) {
			memcpy(buf.cpu_mem, data, size);
		}
	}
}

static void apply_viewport() {
	if (!rsx_ctx) return;
	float scale[4] = { g_vp_w * 0.5f, g_vp_h * -0.5f, (g_z_far - g_z_near) * 0.5f, 0.0f };
	float offset[4] = { g_vp_x + g_vp_w * 0.5f, g_vp_y + g_vp_h * 0.5f, (g_z_far + g_z_near) * 0.5f, 0.0f };
	rsxSetViewport(rsx_ctx, g_vp_x, g_vp_y, g_vp_w, g_vp_h, g_z_near, g_z_far, scale, offset);
}

extern "C" {
	void MultiplyMatrices4by4OpenGL_FLOAT(float *result, const float *matrix, const float *pvector) {
		result[0]=matrix[0]*pvector[0]+matrix[4]*pvector[1]+matrix[8]*pvector[2]+matrix[12]*pvector[3];
		result[4]=matrix[0]*pvector[4]+matrix[4]*pvector[5]+matrix[8]*pvector[6]+matrix[12]*pvector[7];
		result[8]=matrix[0]*pvector[8]+matrix[4]*pvector[9]+matrix[8]*pvector[10]+matrix[12]*pvector[11];
		result[12]=matrix[0]*pvector[12]+matrix[4]*pvector[13]+matrix[8]*pvector[14]+matrix[12]*pvector[15];
		result[1]=matrix[1]*pvector[0]+matrix[5]*pvector[1]+matrix[9]*pvector[2]+matrix[13]*pvector[3];
		result[5]=matrix[1]*pvector[4]+matrix[5]*pvector[5]+matrix[9]*pvector[6]+matrix[13]*pvector[7];
		result[9]=matrix[1]*pvector[8]+matrix[5]*pvector[9]+matrix[9]*pvector[10]+matrix[13]*pvector[11];
		result[13]=matrix[1]*pvector[12]+matrix[5]*pvector[13]+matrix[9]*pvector[14]+matrix[13]*pvector[15];
		result[2]=matrix[2]*pvector[0]+matrix[6]*pvector[1]+matrix[10]*pvector[2]+matrix[14]*pvector[3];
		result[6]=matrix[2]*pvector[4]+matrix[6]*pvector[5]+matrix[10]*pvector[6]+matrix[14]*pvector[7];
		result[10]=matrix[2]*pvector[8]+matrix[6]*pvector[9]+matrix[10]*pvector[10]+matrix[14]*pvector[11];
		result[14]=matrix[2]*pvector[12]+matrix[6]*pvector[13]+matrix[10]*pvector[14]+matrix[14]*pvector[15];
		result[3]=matrix[3]*pvector[0]+matrix[7]*pvector[1]+matrix[11]*pvector[2]+matrix[15]*pvector[3];
		result[7]=matrix[3]*pvector[4]+matrix[7]*pvector[5]+matrix[11]*pvector[6]+matrix[15]*pvector[7];
		result[11]=matrix[3]*pvector[8]+matrix[7]*pvector[9]+matrix[11]*pvector[10]+matrix[15]*pvector[11];
		result[15]=matrix[3]*pvector[12]+matrix[7]*pvector[13]+matrix[11]*pvector[14]+matrix[15]*pvector[15];
	}

	void MultiplyMatrixByVector4by4OpenGL_FLOAT(float *resultvector, const float *matrix, const float *pvector) {
		resultvector[0]=matrix[0]*pvector[0]+matrix[4]*pvector[1]+matrix[8]*pvector[2]+matrix[12]*pvector[3];
		resultvector[1]=matrix[1]*pvector[0]+matrix[5]*pvector[1]+matrix[9]*pvector[2]+matrix[13]*pvector[3];
		resultvector[2]=matrix[2]*pvector[0]+matrix[6]*pvector[1]+matrix[10]*pvector[2]+matrix[14]*pvector[3];
		resultvector[3]=matrix[3]*pvector[0]+matrix[7]*pvector[1]+matrix[11]*pvector[2]+matrix[15]*pvector[3];
	}

	#define MAT(m,r,c) (m)[(c)*4+(r)]
	#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
	int glhInvertMatrixf2(float *m, float *out) {
		float wtmp[4][8]; float m0, m1, m2, m3, s; float *r0, *r1, *r2, *r3;
		r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
		r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1), r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3), r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
		r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1), r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3), r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
		r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1), r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3), r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
		r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1), r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3), r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
		if (fabsf(r3[0]) > fabsf(r2[0])) SWAP_ROWS_FLOAT(r3, r2);
		if (fabsf(r2[0]) > fabsf(r1[0])) SWAP_ROWS_FLOAT(r2, r1);
		if (fabsf(r1[0]) > fabsf(r0[0])) SWAP_ROWS_FLOAT(r1, r0);
		if (0.0 == r0[0]) return 0;
		m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
		s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
		s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
		s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
		s = r0[4]; if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
		s = r0[5]; if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
		s = r0[6]; if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
		s = r0[7]; if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }
		if (fabsf(r3[1]) > fabsf(r2[1])) SWAP_ROWS_FLOAT(r3, r2);
		if (fabsf(r2[1]) > fabsf(r1[1])) SWAP_ROWS_FLOAT(r2, r1);
		if (0.0 == r1[1]) return 0;
		m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1]; r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2]; r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
		s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
		s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
		s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
		s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }
		if (fabsf(r3[2]) > fabsf(r2[2])) SWAP_ROWS_FLOAT(r3, r2);
		if (0.0 == r2[2]) return 0;
		m3 = r3[2] / r2[2];
		r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4], r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
		if (0.0 == r3[3]) return 0;
		s = 1.0 / r3[3]; r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s; m2 = r2[3]; s = 1.0 / r2[2];
		r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2), r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2); m1 = r1[3];
		r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1, r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1; m0 = r0[3];
		r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0, r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0; m1 = r1[2]; s = 1.0 / r1[1];
		r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r2[5] - r3[5] * m1), r1[6] = s * (r2[6] - r3[6] * m1), r1[7] = s * (r2[7] - r3[7] * m1); m0 = r0[2];
		r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0, r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0; m0 = r0[1]; s = 1.0 / r0[0];
		r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0), r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
		MAT(out, 0, 0) = r0[4]; MAT(out, 0, 1) = r0[5]; MAT(out, 0, 2) = r0[6]; MAT(out, 0, 3) = r0[7];
		MAT(out, 1, 0) = r1[4]; MAT(out, 1, 1) = r1[5]; MAT(out, 1, 2) = r1[6]; MAT(out, 1, 3) = r1[7];
		MAT(out, 2, 0) = r2[4]; MAT(out, 2, 1) = r2[5]; MAT(out, 2, 2) = r2[6]; MAT(out, 2, 3) = r2[7];
		MAT(out, 3, 0) = r3[4]; MAT(out, 3, 1) = r3[5]; MAT(out, 3, 2) = r3[6]; MAT(out, 3, 3) = r3[7];
		return 1;
	}

	int glhUnProjectf(float winx, float winy, float winz, float *modelview, float *projection, int *viewport, float *objectCoordinate) {
		float m[16], A[16]; float in[4], out[4]; MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
		if(glhInvertMatrixf2(A, m)==0) return 0;
		in[0]=(winx-(float)viewport[0])/(float)viewport[2]*2.0-1.0;
		in[1]=(winy-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
		in[2]=2.0*winz-1.0; in[3]=1.0; MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
		if(out[3]==0.0f) return 0;
		out[3]=1.0f/out[3];
		objectCoordinate[0]=out[0]*out[3]; objectCoordinate[1]=out[1]*out[3]; objectCoordinate[2]=out[2]*out[3];
		return 1;
	}

	void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers) = impl_glDeleteBuffers;
	void (*glGenBuffers)(GLsizei n, GLuint* buffers) = impl_glGenBuffers;
	void (*glBindBuffer)(GLenum target, GLuint buffer) = impl_glBindBuffer;
	void (*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) = impl_glBufferData;

	void initGlFuncs(void) {
		if (g_rsx_initialized) return;

		g_vpo = (rsxVertexProgram*)vertex_vcg_vpo;
		g_fpo = (rsxFragmentProgram*)fragment_fcg_fpo;

		u32 vp_size = 0, fpo_size = 0;
		rsxVertexProgramGetUCode(g_vpo, &g_vp_ucode, &vp_size);
		rsxFragmentProgramGetUCode(g_fpo, &g_fp_ucode, &fpo_size);

		g_fp_rsx_mem = rsxMemalign(64, fpo_size);
		memcpy(g_fp_rsx_mem, g_fp_ucode, fpo_size);
		cpu_cache_flush(g_fp_rsx_mem, fpo_size);
		rsxAddressToOffset(g_fp_rsx_mem, &g_fp_offset);

		g_param_mvp = rsxVertexProgramGetConst(g_vpo, "mvp");
		g_param_u_color = rsxVertexProgramGetConst(g_vpo, "u_color");
		g_param_use_vtx_color = rsxVertexProgramGetConst(g_vpo, "u_use_vtx_color");
		g_param_u_flags = rsxVertexProgramGetConst(g_vpo, "u_flags");

		g_dynamic_vtx_mem = rsxMemalign(128, DYNAMIC_RSX_VTX_SIZE);
		rsxAddressToOffset(g_dynamic_vtx_mem, &g_dynamic_vtx_offset);

		rsxSetClearDepthStencil(rsx_ctx, 0xffffff00);

		rsxSetDepthWriteEnable(rsx_ctx, GCM_TRUE);
		rsxSetDepthTestEnable(rsx_ctx, GCM_TRUE);
		g_depth_test_enabled = true;
		g_depth_write_enabled = true;
		g_depth_func = GCM_LEQUAL;

		rsxSetCullFaceEnable(rsx_ctx, GCM_TRUE);
		g_cull_face_enabled = true;

		rsxSetAlphaTestEnable(rsx_ctx, GCM_FALSE);
		g_alpha_test_enabled = false;

		rsxSetBlendEnable(rsx_ctx, GCM_FALSE);
		g_blend_enabled = false;

		// --- FIX STENCIL PARA NV40 (PS3) ---
		rsxSetStencilTestEnable(rsx_ctx, GCM_FALSE);
		rsxSetTwoSidedStencilTestEnable(rsx_ctx, GCM_FALSE);
		g_stencil_test_enabled = false;
		rsxSetStencilFunc(rsx_ctx, GCM_ALWAYS, 0, 0xFFu);
		rsxSetBackStencilFunc(rsx_ctx, GCM_ALWAYS, 0, 0xFFu);
		rsxSetStencilMask(rsx_ctx, 0xFFu);
		rsxSetStencilOp(rsx_ctx, GCM_KEEP, GCM_KEEP, GCM_KEEP);
		rsxSetBackStencilOp(rsx_ctx, GCM_KEEP, GCM_KEEP, GCM_KEEP);
		rsxSetPolygonOffsetFillEnable(rsx_ctx, GCM_FALSE);
		g_poly_offset_fill = false;
		// -----------------------------------

		rsxSetDepthFunc(rsx_ctx, GCM_LEQUAL);
		rsxSetBlendFunc(rsx_ctx, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA);
		rsxSetBlendEquation(rsx_ctx, GCM_FUNC_ADD, GCM_FUNC_ADD);

		rsxLoadVertexProgram(rsx_ctx, g_vpo, g_vp_ucode);
		rsxLoadFragmentProgramLocation(rsx_ctx, g_fpo, g_fp_offset, GCM_LOCATION_RSX);

		rsxSetFrontFace(rsx_ctx, GCM_FRONTFACE_CW);
		rsxSetCullFace(rsx_ctx, GCM_CULL_BACK);

		g_rsx_initialized = true;
	}

	void glClearColor(float r, float g, float b, float a) {
		uint8_t ur = (uint8_t)(r * 255.0f), ug = (uint8_t)(g * 255.0f);
		uint8_t ub = (uint8_t)(b * 255.0f), ua = (uint8_t)(a * 255.0f);
		clear_color_packed = (ua << 24) | (ur << 16) | (ug << 8) | ub;
		if (rsx_ctx) rsxSetClearColor(rsx_ctx, clear_color_packed);
	}

	void glClear(unsigned int mask) {
		if(!g_rsx_initialized) initGlFuncs();
		if (!rsx_ctx) return;

		u32 rsx_mask = 0;
		if (mask & 0x4000) rsx_mask |= (GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A);
		if (mask & 0x0100) rsx_mask |= GCM_CLEAR_Z;
		if (mask & 0x0400) rsx_mask |= GCM_CLEAR_S;
		if (rsx_mask != 0) {
			rsxSetClearDepthStencil(rsx_ctx, 0xffffff00); // 0xffffff = profundidad 1.0, 00 = stencil 0
			rsxClearSurface(rsx_ctx, rsx_mask);
		}
	}

	void glMatrixMode(unsigned int mode) { current_matrix_mode = mode; }

	void glLoadIdentity(void) {
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_identity();
		else stack_modelview.back() = mat_identity();
	}
	void glPushMatrix(void) {
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.push_back(stack_projection.back());
		else stack_modelview.push_back(stack_modelview.back());
	}
	void glPopMatrix(void) {
		if ((current_matrix_mode == 0x1701 || current_matrix_mode == 5889) && stack_projection.size() > 1) stack_projection.pop_back();
		else if (stack_modelview.size() > 1) stack_modelview.pop_back();
	}

	void glOrthof(float l, float r, float b, float t, float n, float f) {
		Mat4 o = mat_identity();
		o.m[0] = 2.0f / (r - l);
		o.m[5] = 2.0f / (t - b);
		o.m[10] = -2.0f / (f - n);
		o.m[12] = -(r + l) / (r - l);
		o.m[13] = -(t + b) / (t - b);
		o.m[14] = -(f + n) / (f - n);
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_mul(stack_projection.back(), o);
		else stack_modelview.back() = mat_mul(stack_modelview.back(), o);
	}

	void glOrtho(double l, double r, double b, double t, double n, double f) {
		glOrthof((float)l, (float)r, (float)b, (float)t, (float)n, (float)f);
	}

	void gluPerspective(float fovy, float aspect, float zNear, float zFar) {
		float f = 1.0f / tanf(fovy * (M_PI / 360.0f));
		Mat4 persp = mat_identity();
		persp.m[0] = f / aspect;
		persp.m[5] = f;
		persp.m[10] = (zFar + zNear) / (zNear - zFar);
		persp.m[11] = -1.0f;
		persp.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
		persp.m[15] = 0.0f;
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_mul(stack_projection.back(), persp);
		else stack_modelview.back() = mat_mul(stack_modelview.back(), persp);
	}

	void glTranslatef(float x, float y, float z) {
		Mat4 tr = mat_identity(); tr.m[12] = x; tr.m[13] = y; tr.m[14] = z;
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_mul(stack_projection.back(), tr);
		else stack_modelview.back() = mat_mul(stack_modelview.back(), tr);
	}

	void glScalef(float x, float y, float z) {
		Mat4 sc = mat_identity(); sc.m[0] = x; sc.m[5] = y; sc.m[10] = z;
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_mul(stack_projection.back(), sc);
		else stack_modelview.back() = mat_mul(stack_modelview.back(), sc);
	}

	void glRotatef(float angle, float x, float y, float z) {
		float rad = angle * 3.141592653589793f / 180.0f;
		float c = cosf(rad), s = sinf(rad), len = sqrtf(x*x + y*y + z*z);
		if (len == 0.0f) return;
		x /= len; y /= len; z /= len;
		Mat4 rot = mat_identity();
		rot.m[0] = x*x*(1-c) + c;     rot.m[4] = x*y*(1-c) - z*s;   rot.m[8] = x*z*(1-c) + y*s;
		rot.m[1] = y*x*(1-c) + z*s;   rot.m[5] = y*y*(1-c) + c;     rot.m[9] = y*z*(1-c) - x*s;
		rot.m[2] = x*z*(1-c) - y*s;   rot.m[6] = y*z*(1-c) + x*s;   rot.m[10]= z*z*(1-c) + c;
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_mul(stack_projection.back(), rot);
		else stack_modelview.back() = mat_mul(stack_modelview.back(), rot);
	}

	void glMultMatrixf(const float *m) {
		Mat4 custom; memcpy(custom.m, m, sizeof(custom.m));
		if (current_matrix_mode == 0x1701 || current_matrix_mode == 5889) stack_projection.back() = mat_mul(stack_projection.back(), custom);
		else stack_modelview.back() = mat_mul(stack_modelview.back(), custom);
	}

	void glGetFloatv(unsigned int pname, float *data) {
		if (!data) return;
		if (pname == 0xBA7 || pname == 2983) memcpy(data, stack_projection.back().m, 16 * sizeof(float));
		else if (pname == 0xBA6 || pname == 2982) memcpy(data, stack_modelview.back().m, 16 * sizeof(float));
	}

	void glColor4f(float r, float g, float b, float a) {
		cur_r = (uint8_t)(r * 255.0f); cur_g = (uint8_t)(g * 255.0f);
		cur_b = (uint8_t)(b * 255.0f); cur_a = (uint8_t)(a * 255.0f);
	}

	void glVertexPointer(int s, unsigned int t, int st, const void *p) {
		vtx_ptr = (const float*)p; size_vtx = s; type_vtx = t; vtx_stride = st; g_vtx_vbo = g_bound_vbo;
	}
	void glColorPointer(int s, unsigned int t, int st, const void *p) {
		col_ptr = p; size_color = s; col_stride = st; type_color = t; g_col_vbo = g_bound_vbo;
	}
	void glTexCoordPointer(int s, unsigned int t, int st, const void *p) {
		tex_coord_ptr = (const float*)p; size_tex_coord = s; type_tex_coord = t; tex_coord_stride = st; g_tex_vbo = g_bound_vbo;
	}

	void glNormalPointer(unsigned int type, int stride, const void *pointer) {
		norm_ptr = pointer; type_norm = type; norm_stride = stride;
		g_norm_vbo = g_bound_vbo;
	}

	void glEnableClientState(unsigned int cap) {
		if (cap == 32884 || cap == 0x8074) vtx_enabled = true;
		if (cap == 32886 || cap == 0x8076) col_enabled = true;
		if (cap == 32888 || cap == 0x8078) tex_enabled = true;
		if (cap == 32885 || cap == 0x8075) norm_enabled = true;
	}
	void glDisableClientState(unsigned int cap) {
		if (cap == 32884 || cap == 0x8074) vtx_enabled = false;
		if (cap == 32886 || cap == 0x8076) col_enabled = false;
		if (cap == 32888 || cap == 0x8078) tex_enabled = false;
		if (cap == 32885 || cap == 0x8075) norm_enabled = false;
	}

	void glEnable(unsigned int cap) {
		if(!g_rsx_initialized) initGlFuncs();
		if (!rsx_ctx) return;
		if (cap == 3042 || cap == 0x0BE2) { g_blend_enabled = true; rsxSetBlendEnable(rsx_ctx, GCM_TRUE); }
		if (cap == 2929 || cap == 0x0B71) { g_depth_test_enabled = true; } // FIX ZROP
		if (cap == 3008 || cap == 0x0BC0) { g_alpha_test_enabled = true; rsxSetAlphaTestEnable(rsx_ctx, GCM_TRUE); }
		if (cap == 2884 || cap == 0x0B44) { g_cull_face_enabled = true; rsxSetCullFaceEnable(rsx_ctx, GCM_TRUE); }
		if (cap == 3553 || cap == 0x0DE1) { g_texture_2d_enabled = true; }

		if (cap == 2896 || cap == 0x0B50) { g_lighting_enabled = true; }

		// --- FIX STENCIL PARA NV40 ---
		if (cap == 2960 || cap == 0x0B90) {
			g_stencil_test_enabled = true;
			rsxSetStencilTestEnable(rsx_ctx, GCM_TRUE);
			rsxSetTwoSidedStencilTestEnable(rsx_ctx, GCM_TRUE); // Magia para PS3
		}
		if (cap == 32823 || cap == 0x8037) {
			g_poly_offset_fill = true;
			rsxSetPolygonOffsetFillEnable(rsx_ctx, GCM_TRUE);
		}

		if (cap == 3089 || cap == 0x0C11) {
			g_scissor_enabled = true;
			rsxSetScissor(rsx_ctx, (u16)g_scissor_x, (u16)g_scissor_y, (u16)g_scissor_w, (u16)g_scissor_h);
		}
	}

	void glDisable(unsigned int cap) {
		if(!g_rsx_initialized) initGlFuncs();
		if (!rsx_ctx) return;
		if (cap == 3042 || cap == 0x0BE2) { g_blend_enabled = false; rsxSetBlendEnable(rsx_ctx, GCM_FALSE); }
		if (cap == 2929 || cap == 0x0B71) { g_depth_test_enabled = false; } // FIX ZROP
		if (cap == 3008 || cap == 0x0BC0) { g_alpha_test_enabled = false; rsxSetAlphaTestEnable(rsx_ctx, GCM_FALSE); }
		if (cap == 2884 || cap == 0x0B44) { g_cull_face_enabled = false; rsxSetCullFaceEnable(rsx_ctx, GCM_FALSE); }
		if (cap == 3553 || cap == 0x0DE1) { g_texture_2d_enabled = false; }

		if (cap == 2896 || cap == 0x0B50) { g_lighting_enabled = false; }

		// --- FIX STENCIL PARA NV40 ---
		if (cap == 2960 || cap == 0x0B90) {
			g_stencil_test_enabled = false;
			rsxSetStencilTestEnable(rsx_ctx, GCM_FALSE);
			rsxSetTwoSidedStencilTestEnable(rsx_ctx, GCM_FALSE);
		}
		if (cap == 32823 || cap == 0x8037) {
			g_poly_offset_fill = false;
			rsxSetPolygonOffsetFillEnable(rsx_ctx, GCM_FALSE);
		}

		if (cap == 3089 || cap == 0x0C11) {
			g_scissor_enabled = false;
			rsxSetScissor(rsx_ctx, 0, 0, 4096, 4096);
		}
	}

	void glDepthMask(unsigned char flag) {
		if(!g_rsx_initialized) initGlFuncs();
		g_depth_write_enabled = (flag != 0);
		if (rsx_ctx) rsxSetDepthWriteEnable(rsx_ctx, g_depth_write_enabled ? GCM_TRUE : GCM_FALSE);
	}

	void glColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
		if(!g_rsx_initialized) initGlFuncs();
		if (rsx_ctx) {
			g_color_mask = 0;
			if (red) g_color_mask |= GCM_COLOR_MASK_R;
			if (green) g_color_mask |= GCM_COLOR_MASK_G;
			if (blue) g_color_mask |= GCM_COLOR_MASK_B;
			if (alpha) g_color_mask |= GCM_COLOR_MASK_A;
			rsxSetColorMask(rsx_ctx, g_color_mask);
		}
	}

	static uint32_t mapCompareFunc(unsigned int f) {
		switch(f) {
			case 0x0200: return GCM_NEVER;
			case 0x0201: return GCM_LESS;
			case 0x0202: return GCM_EQUAL;
			case 0x0203: return GCM_LEQUAL;
			case 0x0204: return GCM_GREATER;
			case 0x0205: return GCM_NOTEQUAL;
			case 0x0206: return GCM_GEQUAL;
			case 0x0207: return GCM_ALWAYS;
			default: return GCM_ALWAYS;
		}
	}

	static uint32_t mapStencilOp(unsigned int op) {
		switch(op) {
			case 0x0000: return GCM_ZERO;
			case 0x1E00: return GCM_KEEP;
			case 0x1E01: return GCM_REPLACE;
			case 0x1E02: return GCM_INCR;
			case 0x1E03: return GCM_DECR;
			case 0x150A: return GCM_INVERT;
			case 0x8507: return GCM_INCR_WRAP;
			case 0x8508: return GCM_DECR_WRAP;
			default: return GCM_KEEP;
		}
	}

	void glStencilFunc(unsigned int func, int ref, unsigned int mask) {
		if(!g_rsx_initialized) initGlFuncs();
		g_stencil_func = mapCompareFunc(func);
		g_stencil_ref = ref;
		g_stencil_mask = mask;
		if (rsx_ctx) {
			rsxSetStencilFunc(rsx_ctx, g_stencil_func, g_stencil_ref, g_stencil_mask);
			rsxSetBackStencilFunc(rsx_ctx, g_stencil_func, g_stencil_ref, g_stencil_mask); // Para RSX NV40
		}
	}

	void glStencilOp(unsigned int fail, unsigned int zfail, unsigned int zpass) {
		if(!g_rsx_initialized) initGlFuncs();
		g_stencil_fail = mapStencilOp(fail);
		g_stencil_zfail = mapStencilOp(zfail);
		g_stencil_zpass = mapStencilOp(zpass);
		if (rsx_ctx) {
			rsxSetStencilOp(rsx_ctx, g_stencil_fail, g_stencil_zfail, g_stencil_zpass);
			rsxSetBackStencilOp(rsx_ctx, g_stencil_fail, g_stencil_zfail, g_stencil_zpass); // Para RSX NV40
		}
	}

	void glAlphaFunc(unsigned int func, float ref) {
		if(!g_rsx_initialized) initGlFuncs();
		g_alpha_ref = ref;
		if (rsx_ctx) {
			uint32_t ref_int = (uint32_t)(ref * 255.0f);
			if (ref_int > 255) ref_int = 255;
			rsxSetAlphaFunc(rsx_ctx, mapCompareFunc(func), ref_int);
		}
	}

	void glDepthFunc(unsigned int func) {
		if(!g_rsx_initialized) initGlFuncs();
		g_depth_func = mapCompareFunc(func);
	}

	void glGenTextures(int n, unsigned int *textures) {
		for (int i = 0; i < n; i++) textures[i] = g_next_tex_id++;
	}

	void glBindTexture(unsigned int target, unsigned int texture) {
		g_bound_tex = texture;
	}

	void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void *pixels) {
		if (g_bound_tex != 0 && width > 0 && height > 0) {
			RSXTexture& t = g_tex_map[g_bound_tex];
			u32 pitch = (width * 4 + 63) & ~63;
			u32 required_size = pitch * height;

			if (!t.rsx_mem || t.size < required_size) {
				if (t.rsx_mem) rsxFree(t.rsx_mem);
				t.rsx_mem = rsxMemalign(128, required_size);
				t.size = required_size;
				rsxAddressToOffset(t.rsx_mem, &t.offset);
			}
			t.w = width; t.h = height;

			if (pixels) {
				copy_pixels((uint8_t*)t.rsx_mem, (const uint8_t*)pixels, 0, 0, width, height, pitch, width, height, format, type);
				cpu_cache_flush(t.rsx_mem, t.size);
			}

			memset(&t.tex, 0, sizeof(gcmTexture));
			t.tex.format = GCM_TEXTURE_FORMAT_A8R8G8B8 | GCM_TEXTURE_FORMAT_LIN;
			t.tex.mipmap = 1; t.tex.dimension = GCM_TEXTURE_DIMS_2D;
			t.tex.location = GCM_LOCATION_RSX; t.tex.width = width;
			t.tex.height = height; t.tex.pitch = pitch; t.tex.offset = t.offset;
			t.tex.remap = ((GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_B_SHIFT) | (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_G_SHIFT) | (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_R_SHIFT) | (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_A_SHIFT) | (GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_B_SHIFT) | (GCM_TEXTURE_REMAP_COLOR_G << GCM_TEXTURE_REMAP_COLOR_G_SHIFT) | (GCM_TEXTURE_REMAP_COLOR_R << GCM_TEXTURE_REMAP_COLOR_R_SHIFT) | (GCM_TEXTURE_REMAP_COLOR_A << GCM_TEXTURE_REMAP_COLOR_A_SHIFT));
			rsxInvalidateTextureCache(rsx_ctx, GCM_INVALIDATE_TEXTURE);
		}
	}

	void glTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *pixels) {
		if (g_bound_tex != 0 && pixels && width > 0 && height > 0) {
			RSXTexture& t = g_tex_map[g_bound_tex];
			if (!t.rsx_mem || t.w <= 0 || t.h <= 0) return;
			copy_pixels((uint8_t*)t.rsx_mem, (const uint8_t*)pixels, xoffset, yoffset, width, height, t.tex.pitch, t.w, t.h, format, type);
			cpu_cache_flush(t.rsx_mem, t.size);
			rsxInvalidateTextureCache(rsx_ctx, GCM_INVALIDATE_TEXTURE);
		}
	}

	void glDeleteTextures(int n, const unsigned int *textures) {
		if (!textures) return;
		for (int i = 0; i < n; i++) {
			auto it = g_tex_map.find(textures[i]);
			if (it != g_tex_map.end()) {
				if (it->second.rsx_mem) rsxFree(it->second.rsx_mem);
				g_tex_map.erase(it);
			}
			if (g_bound_tex == textures[i]) g_bound_tex = 0;
		}
	}

	static const uint8_t* get_client_ptr(GLuint vbo, const void* ptr) {
		if (vbo != 0) {
			auto it = g_vbo_map.find(vbo);
			if (it != g_vbo_map.end() && it->second.cpu_mem) {
				return (const uint8_t*)it->second.cpu_mem + (uintptr_t)ptr;
			}
			return NULL;
		}
		return (const uint8_t*)ptr;
	}

	static void internal_draw(unsigned int mode, int first, int count, int use_elements, unsigned int type, const void* indices) {
		if (!rsx_ctx || count <= 0) return;
		if (!g_rsx_initialized) initGlFuncs();

		dbg_draws++;
		dbg_verts += count;

		Mat4 mvp = mat_mul(stack_projection.back(), stack_modelview.back());
		Mat4 mvp_trans = mat_transpose(mvp);

		if (is_matrix_corrupt(mvp_trans)) dbg_nan = true;

		if (g_param_mvp) rsxSetVertexProgramParameter(rsx_ctx, g_vpo, g_param_mvp, mvp_trans.m);

		// --- FIX ZROP (STENCIL BUG EN RSX NV40) ---
		if (g_depth_test_enabled) {
			rsxSetDepthTestEnable(rsx_ctx, GCM_TRUE);
			rsxSetDepthFunc(rsx_ctx, g_depth_func);
		} else {
			if (g_stencil_test_enabled) {
				// Si la profundidad esta "apagada" en OpenGL pero el Stencil esta encendido,
				// la PS3 apagaria el hardware de ambos. Forzamos a que el ZROP siga vivo.
				rsxSetDepthTestEnable(rsx_ctx, GCM_TRUE);
				rsxSetDepthFunc(rsx_ctx, GCM_ALWAYS);
			} else {
				rsxSetDepthTestEnable(rsx_ctx, GCM_FALSE);
			}
		}
		rsxSetDepthWriteEnable(rsx_ctx, g_depth_write_enabled ? GCM_TRUE : GCM_FALSE);
		rsxSetCullFaceEnable(rsx_ctx, g_cull_face_enabled ? GCM_TRUE : GCM_FALSE);
		rsxSetBlendEnable(rsx_ctx, g_blend_enabled ? GCM_TRUE : GCM_FALSE);
		rsxSetAlphaTestEnable(rsx_ctx, g_alpha_test_enabled ? GCM_TRUE : GCM_FALSE);
		rsxSetStencilTestEnable(rsx_ctx, g_stencil_test_enabled ? GCM_TRUE : GCM_FALSE);
		rsxSetTwoSidedStencilTestEnable(rsx_ctx, g_stencil_test_enabled ? GCM_TRUE : GCM_FALSE);
		rsxSetStencilFunc(rsx_ctx, g_stencil_func, g_stencil_ref, g_stencil_mask);
		rsxSetBackStencilFunc(rsx_ctx, g_stencil_func, g_stencil_ref, g_stencil_mask);
		rsxSetStencilOp(rsx_ctx, g_stencil_fail, g_stencil_zfail, g_stencil_zpass);
		rsxSetBackStencilOp(rsx_ctx, g_stencil_fail, g_stencil_zfail, g_stencil_zpass);
		rsxSetPolygonOffsetFillEnable(rsx_ctx, g_poly_offset_fill ? GCM_TRUE : GCM_FALSE);
		rsxSetColorMask(rsx_ctx, g_color_mask);
		// -------------------------------------------

		float u_color_vec[4] = { cur_r / 255.0f, cur_g / 255.0f, cur_b / 255.0f, cur_a / 255.0f };
		float use_vtx_col_vec[4] = { col_enabled ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
		bool has_tex = (g_texture_2d_enabled && g_bound_tex != 0 && g_tex_map.find(g_bound_tex) != g_tex_map.end());
		float u_flags_vec[4] = { has_tex ? 1.0f : 0.0f, g_alpha_test_enabled ? 1.0f : 0.0f, g_alpha_ref, 0.0f };

		if (g_param_u_color) rsxSetVertexProgramParameter(rsx_ctx, g_vpo, g_param_u_color, u_color_vec);
		if (g_param_use_vtx_color) rsxSetVertexProgramParameter(rsx_ctx, g_vpo, g_param_use_vtx_color, use_vtx_col_vec);
		if (g_param_u_flags) rsxSetVertexProgramParameter(rsx_ctx, g_vpo, g_param_u_flags, u_flags_vec);

		if (has_tex) {
			RSXTexture& tex = g_tex_map[g_bound_tex];
			rsxLoadTexture(rsx_ctx, 0, &tex.tex);
			rsxTextureControl(rsx_ctx, 0, GCM_TRUE, 0, 0, GCM_TEXTURE_MAX_ANISO_1);
			rsxTextureFilter(rsx_ctx, 0, 0, GCM_TEXTURE_NEAREST, GCM_TEXTURE_NEAREST, GCM_TEXTURE_CONVOLUTION_QUINCUNX);
			rsxTextureWrapMode(rsx_ctx, 0, GCM_TEXTURE_REPEAT, GCM_TEXTURE_REPEAT, GCM_TEXTURE_CLAMP_TO_EDGE, 0, GCM_TEXTURE_ZFUNC_LESS, 0);
		} else {
			rsxTextureControl(rsx_ctx, 0, GCM_FALSE, 0, 0, GCM_TEXTURE_MAX_ANISO_1);
		}

		uint32_t rsx_mode = GCM_TYPE_TRIANGLES;
		if (mode == 0 || mode == 0x0000) rsx_mode = GCM_TYPE_POINTS;
		else if (mode == 1 || mode == 0x0001) rsx_mode = GCM_TYPE_LINES;
		else if (mode == 3 || mode == 0x0003) rsx_mode = GCM_TYPE_LINE_STRIP;
		else if (mode == 5 || mode == 0x0005) rsx_mode = GCM_TYPE_TRIANGLE_STRIP;
		else if (mode == 6 || mode == 0x0006) rsx_mode = GCM_TYPE_TRIANGLE_FAN;
		else if (mode == 7 || mode == 0x0007) rsx_mode = GCM_TYPE_QUADS;

		const uint8_t* actual_vtx_ptr = get_client_ptr(g_vtx_vbo, vtx_ptr);
		const uint8_t* actual_tex_ptr = get_client_ptr(g_tex_vbo, tex_coord_ptr);
		const uint8_t* actual_col_ptr = get_client_ptr(g_col_vbo, col_ptr);
		const uint8_t* actual_norm_ptr = get_client_ptr(g_norm_vbo, norm_ptr);
		const uint8_t* actual_idx_ptr = get_client_ptr(g_bound_ibo, indices);

		u32 required_bytes = count * 36;

		if (required_bytes >= DYNAMIC_RSX_VTX_SIZE) return;

		g_dynamic_vtx_curr = (g_dynamic_vtx_curr + 127) & ~127;

		if (g_dynamic_vtx_curr + required_bytes >= DYNAMIC_RSX_VTX_SIZE) {
			wait_rsx_idle();
			g_dynamic_vtx_curr = 0;
			dbg_flushes++;
		}

		uint8_t* dst = (uint8_t*)g_dynamic_vtx_mem + g_dynamic_vtx_curr;
		u32 base_offset = g_dynamic_vtx_offset + g_dynamic_vtx_curr;
		g_dynamic_vtx_curr += required_bytes;

		u32 v_str = vtx_stride;
		if (v_str == 0) {
			if (type_vtx == 0x1406 || type_vtx == 0x140C) v_str = size_vtx * 4;
			else if (type_vtx == 0x1402) v_str = size_vtx * 2;
			else if (type_vtx == 0x1400 || type_vtx == 0x1401) v_str = size_vtx;
			else v_str = size_vtx * 4;
		}

		u32 t_str = tex_coord_stride;
		if (t_str == 0) {
			if (type_tex_coord == 0x1406 || type_tex_coord == 0x140C) t_str = size_tex_coord * 4;
			else if (type_tex_coord == 0x1402) t_str = size_tex_coord * 2;
			else if (type_tex_coord == 0x1400 || type_tex_coord == 0x1401) t_str = size_tex_coord;
			else t_str = size_tex_coord * 4;
		}

		u32 c_str = col_stride ? col_stride : ((type_color == 0x1406 || type_color == 5126) ? (size_color * 4) : size_color);

		u32 n_str = norm_stride;
		if (n_str == 0) {
			if (type_norm == 0x1406 || type_norm == 5126) n_str = 12;
			else if (type_norm == 0x1402 || type_norm == 5122) n_str = 6;
			else n_str = 3;
		}

		for (int i = 0; i < count; i++) {
			int v_idx = first + i;
			if (use_elements && actual_idx_ptr) {
				if (type == 5123 || type == 0x1403) v_idx = ((uint16_t*)actual_idx_ptr)[i];
				else if (type == 5121 || type == 0x1401) v_idx = ((uint8_t*)actual_idx_ptr)[i];
				else if (type == 5125 || type == 0x1405) v_idx = ((uint32_t*)actual_idx_ptr)[i];
			}

			if (v_idx > dbg_max_idx) dbg_max_idx = v_idx;

			float* dst_vtx = (float*)(dst + i * 36 + 0);
			if (vtx_enabled && actual_vtx_ptr) {
				if (type_vtx == 0x1406 || type_vtx == 5126) {
					float* fvtx = (float*)(actual_vtx_ptr + v_idx * v_str);
					dst_vtx[0] = fvtx[0]; dst_vtx[1] = fvtx[1]; dst_vtx[2] = (size_vtx >= 3) ? fvtx[2] : 0.0f;
				} else if (type_vtx == 0x1402 || type_vtx == 5122) {
					int16_t* svtx = (int16_t*)(actual_vtx_ptr + v_idx * v_str);
					dst_vtx[0] = svtx[0]; dst_vtx[1] = svtx[1]; dst_vtx[2] = (size_vtx >= 3) ? svtx[2] : 0.0f;
				} else if (type_vtx == 0x1400 || type_vtx == 5120) {
					int8_t* bvtx = (int8_t*)(actual_vtx_ptr + v_idx * v_str);
					dst_vtx[0] = bvtx[0]; dst_vtx[1] = bvtx[1]; dst_vtx[2] = (size_vtx >= 3) ? bvtx[2] : 0.0f;
				} else if (type_vtx == 0x140C || type_vtx == 5132) {
					int32_t* ivtx = (int32_t*)(actual_vtx_ptr + v_idx * v_str);
					dst_vtx[0] = ivtx[0] / 65536.0f; dst_vtx[1] = ivtx[1] / 65536.0f; dst_vtx[2] = (size_vtx >= 3) ? (ivtx[2] / 65536.0f) : 0.0f;
				} else {
					dst_vtx[0] = 0.0f; dst_vtx[1] = 0.0f; dst_vtx[2] = 0.0f;
				}
			} else {
				dst_vtx[0] = 0.0f; dst_vtx[1] = 0.0f; dst_vtx[2] = 0.0f;
			}

			float* dst_uv = (float*)(dst + i * 36 + 12);
			if (tex_enabled && actual_tex_ptr && has_tex) {
				if (type_tex_coord == 0x1406 || type_tex_coord == 5126) {
					float* fuv = (float*)(actual_tex_ptr + v_idx * t_str);
					dst_uv[0] = fuv[0]; dst_uv[1] = fuv[1];
				} else if (type_tex_coord == 0x1402 || type_tex_coord == 5122) {
					int16_t* suv = (int16_t*)(actual_tex_ptr + v_idx * t_str);
					dst_uv[0] = suv[0]; dst_uv[1] = suv[1];
				} else if (type_tex_coord == 0x140C || type_tex_coord == 5132) {
					int32_t* iuv = (int32_t*)(actual_tex_ptr + v_idx * t_str);
					dst_uv[0] = iuv[0] / 65536.0f; dst_uv[1] = iuv[1] / 65536.0f;
				} else {
					dst_uv[0] = 0.0f; dst_uv[1] = 0.0f;
				}
			} else {
				dst_uv[0] = 0.0f; dst_uv[1] = 0.0f;
			}

			float light_factor = 1.0f;
			if (g_lighting_enabled && norm_enabled && actual_norm_ptr) {
				float nx = 0, ny = 1, nz = 0;
				if (type_norm == 0x1406 || type_norm == 5126) {
					float* fnorm = (float*)(actual_norm_ptr + v_idx * n_str);
					nx = fnorm[0]; ny = fnorm[1]; nz = fnorm[2];
				} else if (type_norm == 0x1400 || type_norm == 5120 || type_norm == 0x1401 || type_norm == 5121) {
					int8_t* bnorm = (int8_t*)(actual_norm_ptr + v_idx * n_str);
					nx = bnorm[0]; ny = bnorm[1]; nz = bnorm[2];
				} else if (type_norm == 0x1402 || type_norm == 5122) {
					int16_t* snorm = (int16_t*)(actual_norm_ptr + v_idx * n_str);
					nx = snorm[0]; ny = snorm[1]; nz = snorm[2];
				}

				float abs_nx = fabsf(nx);
				float abs_ny = fabsf(ny);
				float abs_nz = fabsf(nz);

				if (abs_ny > abs_nx && abs_ny > abs_nz) {
					light_factor = (ny > 0.0f) ? 1.0f : 0.5f;
				} else if (abs_nz > abs_nx && abs_nz > abs_ny) {
					light_factor = 0.8f;
				} else {
					light_factor = 0.6f;
				}
			}

			float* dst_col = (float*)(dst + i * 36 + 20);
			if (col_enabled && actual_col_ptr) {
				if (type_color == 5126 || type_color == 0x1406) {
					float* fcol = (float*)(actual_col_ptr + v_idx * c_str);
					dst_col[0] = fcol[0]; dst_col[1] = fcol[1]; dst_col[2] = fcol[2]; dst_col[3] = fcol[3];
				} else {
					uint8_t* bcol = (uint8_t*)(actual_col_ptr + v_idx * c_str);
					dst_col[0] = bcol[0] / 255.0f; dst_col[1] = bcol[1] / 255.0f;
					dst_col[2] = bcol[2] / 255.0f; dst_col[3] = bcol[3] / 255.0f;
				}
			} else {
				dst_col[0] = cur_r / 255.0f; dst_col[1] = cur_g / 255.0f;
				dst_col[2] = cur_b / 255.0f; dst_col[3] = cur_a / 255.0f;
			}

			dst_col[0] *= light_factor;
			dst_col[1] *= light_factor;
			dst_col[2] *= light_factor;
		}

		cpu_cache_flush(dst, required_bytes);

		rsxBindVertexArrayAttrib(rsx_ctx, GCM_VERTEX_ATTRIB_POS, 0, base_offset + 0, 36, 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(rsx_ctx, GCM_VERTEX_ATTRIB_TEX0, 0, base_offset + 12, 36, 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(rsx_ctx, GCM_VERTEX_ATTRIB_COLOR0, 0, base_offset + 20, 36, 4, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

		rsxDrawVertexArray(rsx_ctx, rsx_mode, 0, count);
	}

	void glDrawArrays(unsigned int mode, int first, int count) { internal_draw(mode, first, count, 0, 0, NULL); }
	void glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices) { internal_draw(mode, 0, count, 1, type, indices); }

	static uint32_t mapBlendFunc(unsigned int f) {
		switch(f) {
			case 0: return GCM_ZERO;
			case 1: return GCM_ONE;
			case 0x0300: return GCM_SRC_COLOR;
			case 0x0301: return GCM_ONE_MINUS_SRC_COLOR;
			case 0x0302: return GCM_SRC_ALPHA;
			case 0x0303: return GCM_ONE_MINUS_SRC_ALPHA;
			case 0x0304: return GCM_DST_ALPHA;
			case 0x0305: return GCM_ONE_MINUS_DST_ALPHA;
			case 0x0306: return GCM_DST_COLOR;
			case 0x0307: return GCM_ONE_MINUS_DST_COLOR;
			case 0x0308: return GCM_SRC_ALPHA_SATURATE;
			default: return GCM_ONE;
		}
	}

	void glBlendFunc(unsigned int sfactor, unsigned int dfactor) {
		if(!g_rsx_initialized) initGlFuncs();
		if (rsx_ctx) {
			rsxSetBlendFunc(rsx_ctx, mapBlendFunc(sfactor), mapBlendFunc(dfactor), mapBlendFunc(sfactor), mapBlendFunc(dfactor));
			rsxSetBlendEquation(rsx_ctx, GCM_FUNC_ADD, GCM_FUNC_ADD);
		}
	}

	void glViewport(int x, int y, int width, int height) {
		g_vp_x = x; g_vp_y = y; g_vp_w = width; g_vp_h = height;
		g_screen_height = height;
		apply_viewport();
	}

	void glDepthRangef(float n, float f) {
		g_z_near = n; g_z_far = f;
		apply_viewport();
	}

	void glDepthRange(double n, double f) { glDepthRangef((float)n, (float)f); }

	void glScissor(int x, int y, int width, int height) {
		if(!g_rsx_initialized) initGlFuncs();
		if (x < 0) { width += x; x = 0; }
		if (width < 0) width = 0;
		int sy = g_screen_height - (y + height);
		if (sy < 0) { height += sy; sy = 0; }
		if (height < 0) height = 0;

		g_scissor_x = x; g_scissor_y = sy; g_scissor_w = width; g_scissor_h = height;

		if (g_scissor_enabled && rsx_ctx) {
			rsxSetScissor(rsx_ctx, (u16)g_scissor_x, (u16)g_scissor_y, (u16)g_scissor_w, (u16)g_scissor_h);
		}
	}

	void glFrontFace(unsigned int mode) {
		if(!g_rsx_initialized) initGlFuncs();
		if (!rsx_ctx) return;
		uint32_t rsx_mode = (mode == 2304 || mode == 0x0900) ? GCM_FRONTFACE_CCW : GCM_FRONTFACE_CW;
		rsxSetFrontFace(rsx_ctx, rsx_mode);
	}

	void glCullFace(unsigned int mode) {
		if(!g_rsx_initialized) initGlFuncs();
		if (!rsx_ctx) return;
		if (mode == 1028 || mode == 0x0404) rsxSetCullFace(rsx_ctx, GCM_CULL_FRONT);
		else if (mode == 1029 || mode == 0x0405) rsxSetCullFace(rsx_ctx, GCM_CULL_BACK);
		else rsxSetCullFace(rsx_ctx, GCM_CULL_BACK);
	}
}
