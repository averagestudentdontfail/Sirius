// A struct to hold the properties of a single light ray
typedef struct {
    float4 pos;      // Current position (t, x, y, z)
    float4 vel;      // Current velocity (dt/dλ, dx/dλ, dy/dλ, dz/dλ)
    int terminated;  // A flag to stop tracing
    int sx, sy;      // Screen pixel coordinates
} Ray;

// This is a placeholder. We will generate the metric functions here
// using compiler definitions (-D flags) from our C++ host code.
// For Minkowski space, this would effectively be g_00 = -1, g_11 = 1, etc.
float4 get_metric_diagonal(float4 pos) {
    return (float4)(METRIC_G00, METRIC_G11, METRIC_G22, METRIC_G33);
}


// The main kernel that runs for every pixel on the screen.
__kernel void trace_rays(
    __global Ray* rays,           // Input array of rays, one for each pixel
    __write_only image2d_t outputImage // The output image texture
) {
    int id = get_global_id(0);
    Ray ray = rays[id];

    // --- Physics simulation will go here in the future ---
    // For now, we will just color the pixel based on the initial ray velocity
    // to create a simple skydome effect.

    float3 dir = normalize(ray.vel.xyz);
    float3 color = (float3)(0.0f, 0.0f, 0.0f);

    // A simple blue-to-black gradient for the "sky"
    color.z = max(0.0f, dir.y); // Blue color based on vertical direction
    color.xy = 0.1f; // A little bit of red and green

    // A simple grid pattern to show distortion later
    if (fmod(ray.vel.x * 10.0f, 1.0f) > 0.95f || fmod(ray.vel.y * 10.0f, 1.0f) > 0.95f) {
        color = (float3)(1.0f, 1.0f, 1.0f); // White grid lines
    }

    // Write the final color to the output image
    int2 coords = (int2)(ray.sx, ray.sy);
    write_imagef(outputImage, coords, (float4)(color, 1.0f));
}