# Tracer
My offscreen rendering lab based on PBRT

## Features
### Renderer
- [ ] Whitted
- [ ] DirectLighting
- [x] Path
- [x] BDPT
- [x] SSPM
- [ ] MLT
- [ ] AO

### Shape
- [x] TriangleMesh
- [x] Sphere
- [ ] Quad
- [ ] Disk

### Material
- [x] Phong
- [x] Glass
- [x] Homogeneous/heterogeneous medium
- [x] Metal
- [x] Disney
- [ ] Plastic
- [ ] Substrate?
- [ ] Volume

### Light
- [x] Diffuse
- [x] IBL
- [ ] Spot
- [ ] Point
### Others
- [x] SAH based BVH
- [x] Depth of field
- [x] Film filter
- [x] Multiple importance sampling
- [x] Tone mapping
- [x] BSSRDF
- [ ] SIMD
- [ ] Display
## Screenshot
Veach

![veach-mis](screenshot/tracer_veach-mis.png)

Cornell-box
![cornell-box](screenshot/tracer_cornel-box.png)

Cornell-box with fog

![cornell-box](screenshot/tracer_cornel-box_bdpt_vol_test.png)

bedroom

![bedroom](screenshot/tracer_bedroom.png)

dinningroom

![dinningroom](screenshot/tracer_diningroom.png)

Cornell-box2

![cornell-box](screenshot/tracer_cornellbox.png)

Disney ball

![ball](screenshot/tracer_disney_ball.png)

metallic

![ball](screenshot/disney/tracer_ball_metallic_0.png) ![ball](screenshot/disney/tracer_ball_metallic_0_1.png) ![ball](screenshot/disney/tracer_ball_metallic_0_3.png)
![ball](screenshot/disney/tracer_ball_metallic_0_5.png) ![ball](screenshot/disney/tracer_ball_metallic_0_7.png) ![ball](screenshot/disney/tracer_ball_metallic_0_9.png)
![ball](screenshot/disney/tracer_ball_metallic_1.png)

roughness

![ball](screenshot/disney/tracer_ball_roughness_0.png) ![ball](screenshot/disney/tracer_ball_roughness_0_1.png) ![ball](screenshot/disney/tracer_ball_roughness_0_3.png)
![ball](screenshot/disney/tracer_ball_roughness_0_5.png) ![ball](screenshot/disney/tracer_ball_roughness_0_7.png) ![ball](screenshot/disney/tracer_ball_roughness_0_9.png)
![ball](screenshot/disney/tracer_ball_roughness_1.png)

transmission

![ball](screenshot/disney/tracer_ball_trans_0.png) ![ball](screenshot/disney/tracer_ball_trans_0_3.png) ![ball](screenshot/disney/tracer_ball_trans_0_6.png)
![ball](screenshot/disney/tracer_ball_trans_0_9.png) ![ball](screenshot/disney/tracer_ball_trans_1.png)

ior

![ball](screenshot/disney/tracer_ball_ior_1_5.png) ![ball](screenshot/disney/tracer_ball_ior_2.png) ![ball](screenshot/disney/tracer_ball_ior_2_5.png)
![ball](screenshot/disney/tracer_ball_ior_3.png) ![ball](screenshot/disney/tracer_ball_ior_4.png) ![ball](screenshot/disney/tracer_ball_ior_6.png)

anisotropic

![ball](screenshot/disney/tracer_ball_anisotropic_0.png) ![ball](screenshot/disney/tracer_ball_anisotropic_0_5.png) ![ball](screenshot/disney/tracer_ball_anisotropic_1.png)

sheen

![ball](screenshot/disney/tracer_ball_sheen_0.png) ![ball](screenshot/disney/tracer_ball_sheen_0_5.png) ![ball](screenshot/disney/tracer_ball_sheen_1.png)