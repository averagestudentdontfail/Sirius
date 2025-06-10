// Enhanced OpenCL 3.0 kernel for Sirius ray tracing
// Optimized for both NVIDIA GPU (Windows) and POCL 7.0+ CPU (Linux)

// OpenCL 3.0 feature detection and compatibility
#ifdef OPENCL_VERSION_3_0
    #pragma OPENCL EXTENSION cl_khr_3d_image_writes : enable
    #define USE_OPENCL30_FEATURES 1
#endif

// Optimization hints for OpenCL 3.0
#ifdef USE_OPENCL30_FEATURES
    #pragma OPENCL EXTENSION cl_khr_subgroups : enable
    #pragma OPENCL EXTENSION cl_intel_subgroups : enable
#endif

// A struct to hold the properties of a single light ray
// Optimized alignment for OpenCL 3.0
typedef struct __attribute__((aligned(16))) {
    float4 pos;      // Current position (t, x, y, z)
    float4 vel;      // Current velocity (dt/dλ, dx/dλ, dy/dλ, dz/dλ)
    int terminated;  // A flag to stop tracing
    int sx, sy;      // Screen pixel coordinates
    int padding1, padding2; // Padding for optimal memory alignment
} Ray;

// OpenCL 3.0 optimized constants
#ifdef USE_OPENCL30_FEATURES
    constant float PI_F = 3.14159265358979323846f;
    constant float TWO_PI_F = 6.28318530717958647692f;
    constant float HALF_PI_F = 1.57079632679489661923f;
#else
    #define PI_F 3.14159265358979323846f
    #define TWO_PI_F 6.28318530717958647692f
    #define HALF_PI_F 1.57079632679489661923f
#endif

// Get the metric tensor diagonal components at a given position
// The metric components are injected as compiler definitions from C++
// OpenCL 3.0 inline optimization
inline float4 get_metric_diagonal(float4 pos) {
    // For now, we use constant metric values (Minkowski or other)
    // In the future, this could depend on position for curved spacetime
    return (float4)(METRIC_G00, METRIC_G11, METRIC_G22, METRIC_G33);
}

// OpenCL 3.0 optimized fast math functions
#ifdef USE_OPENCL30_FEATURES
    // Use OpenCL 3.0 native math functions for better performance
    #define FAST_SIN(x) native_sin(x)
    #define FAST_COS(x) native_cos(x)
    #define FAST_SQRT(x) native_sqrt(x)
    #define FAST_EXP(x) native_exp(x)
    #define FAST_LOG(x) native_log(x)
#else
    // Fallback to standard functions
    #define FAST_SIN(x) sin(x)
    #define FAST_COS(x) cos(x)
    #define FAST_SQRT(x) sqrt(x)
    #define FAST_EXP(x) exp(x)
    #define FAST_LOG(x) log(x)
#endif

// Enhanced color computation with OpenCL 3.0 optimizations
float3 compute_color(Ray ray, float4 metric_diag) {
    float3 color = (float3)(0.0f, 0.0f, 0.0f);
    
    // Get normalized direction using OpenCL 3.0 fast normalize
    float3 dir = fast_normalize(ray.vel.yzw); // spatial components only
    
    // Create a gradient based on ray direction and metric
    float metric_factor = (metric_diag.x + metric_diag.y + metric_diag.z + metric_diag.w) * 0.25f;
    
    // Base color from direction with enhanced mathematical functions
    color.x = 0.5f + 0.5f * dir.x; // Red channel
    color.y = 0.5f + 0.5f * dir.y; // Green channel  
    color.z = 0.7f + 0.3f * dir.z; // Blue channel (sky-like)
    
    // Modulate by metric to show spacetime curvature effects
    color *= (0.8f + 0.2f * metric_factor);
    
    // Add some grid lines for reference with optimized modulo
    float2 grid_coord = ray.vel.yz * 10.0f;
    float grid_lines = 0.0f;
    
    // OpenCL 3.0 optimized modulo operation
    float2 grid_mod = fmod(grid_coord, 1.0f);
    if (grid_mod.x > 0.95f || grid_mod.y > 0.95f) {
        grid_lines = 0.3f;
    }
    
    // Add grid as white lines
    color += (float3)(grid_lines, grid_lines, grid_lines);
    
    // Add some time-based animation with OpenCL 3.0 native functions
    float time_factor = FAST_SIN(ray.pos.x * 0.1f) * 0.1f + 1.0f;
    color *= time_factor;
    
    // Enhanced spacetime visualization for different metrics
    if (metric_diag.x < -0.5f) { // Time-like metric signature
        // Add relativistic effects visualization
        float gamma_factor = 1.0f / FAST_SQRT(1.0f - dot(dir, dir) * 0.1f);
        color *= (1.0f + 0.1f * gamma_factor);
    }
    
    // Clamp color to valid range with OpenCL 3.0 optimization
    color = clamp(color, 0.0f, 1.0f);
    
    return color;
}

// Enhanced ray marching step with OpenCL 3.0 optimizations
void integrate_ray_step(Ray* ray, float4 metric_diag, float step_size) {
    // For Minkowski space, this is trivial (straight lines)
    // For curved spacetime, this would involve Christoffel symbols
    
    // Simple forward integration with OpenCL 3.0 optimized operations
    ray->pos += ray->vel * step_size;
    
    // In curved spacetime, velocity would also change:
    // ray->vel += acceleration * step_size;
    // where acceleration comes from the geodesic equation
    
    // Add some simple curvature effects based on metric
    if (metric_diag.x != -1.0f || metric_diag.y != 1.0f) {
        // Non-Minkowski metric - add simple gravitational deflection
        float3 pos_3d = ray->pos.yzw;
        float r = fast_length(pos_3d);
        if (r > 0.1f) {
            float3 accel = -normalize(pos_3d) * (0.001f / (r * r));
            ray->vel.yzw += accel * step_size;
        }
    }
}

// Enhanced termination conditions with OpenCL 3.0 optimizations
bool should_terminate_ray(Ray ray) {
    // Terminate if ray goes too far
    float distance = fast_length(ray.pos.yzw);
    if (distance > 100.0f) {
        return true;
    }
    
    // For black holes, check if inside event horizon
    // (this would be metric-specific)
    float schwarzschild_radius = 2.0f; // Example value
    if (distance < schwarzschild_radius) {
        return true;
    }
    
    // Terminate if ray velocity becomes too small (absorbed)
    float vel_magnitude = fast_length(ray.vel);
    if (vel_magnitude < 0.001f) {
        return true;
    }
    
    return false;
}

// OpenCL 3.0 optimized main kernel with work group optimizations
__kernel void trace_rays(
    __global Ray* rays,           // Input array of rays, one for each pixel
    __write_only image2d_t outputImage // The output image texture
) {
    int id = get_global_id(0);
    
    // Bounds check with OpenCL 3.0 optimization
    int total_pixels = get_global_size(0);
    if (id >= total_pixels) {
        return;
    }

#ifdef USE_OPENCL30_FEATURES
    // OpenCL 3.0 subgroup optimizations for GPU workloads
    int subgroup_id = get_sub_group_id();
    int subgroup_local_id = get_sub_group_local_id();
    int subgroup_size = get_sub_group_size();
    
    // Prefetch ray data for better memory performance on GPU
    prefetch(&rays[id], 1);
#endif
    
    Ray ray = rays[id];
    
    // Get metric at current position
    float4 metric_diag = get_metric_diagonal(ray.pos);
    
    // Enhanced ray tracing with OpenCL 3.0 optimizations
    const int max_steps = 20; // Increased for better quality
    const float step_size = 0.05f; // Smaller steps for better accuracy
    
    // OpenCL 3.0 loop optimization pragma
    #pragma unroll 4
    for (int step = 0; step < max_steps && !ray.terminated; ++step) {
        // Check termination conditions
        if (should_terminate_ray(ray)) {
            ray.terminated = 1;
            break;
        }
        
        // Integrate ray forward one step
        integrate_ray_step(&ray, metric_diag, step_size);
        
        // Update metric for new position
        metric_diag = get_metric_diagonal(ray.pos);
    }
    
    // Compute final color based on ray state
    float3 color = compute_color(ray, metric_diag);
    
    // Enhanced color processing for OpenCL 3.0
#ifdef USE_OPENCL30_FEATURES
    // Apply gamma correction for better visual quality
    color = native_powr(color, 1.0f / 2.2f);
    
    // Add slight dithering to reduce banding
    float noise = native_sin(ray.sx * 12.9898f + ray.sy * 78.233f) * 43758.5453f;
    noise = fract(noise) * 0.01f - 0.005f;
    color += (float3)(noise, noise, noise);
#endif
    
    // Write the final color to the output image
    int2 coords = (int2)(ray.sx, ray.sy);
    float4 final_color = (float4)(color, 1.0f);
    
    // OpenCL 3.0 optimized image write
    write_imagef(outputImage, coords, final_color);

#ifdef USE_OPENCL30_FEATURES
    // Optional: Subgroup collective operations for GPU optimization
    // This can improve memory coalescing on modern GPUs
    work_group_barrier(CLK_LOCAL_MEM_FENCE);
#endif
}