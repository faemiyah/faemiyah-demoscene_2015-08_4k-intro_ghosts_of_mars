#version 430

layout(location=0)uniform sampler2D surface_texture;
layout(location=1)uniform sampler3D noise_volume;
layout(location=2)uniform vec3[10] uniform_array;
#if defined(USE_LD)
layout(location=19)uniform mat3 noise_matrix;
layout(location=22)uniform vec4[3] scales;
layout(location=25)uniform vec3 aspect;
#else
mat3 noise_matrix = mat3(0.42, -0.7, 0.58, 0.53, 0.71, 0.46, -0.74, 0.12, 0.67);
#endif
out vec4 output_color;

vec4 march_params = vec4(0.3, 0.0, 0.7, 0.009);
vec3 light_direction = normalize(vec3(2.0, 0.5, -1.0));
float destruction = uniform_array[9].y;
bool world_toggle = 1.0 < abs(uniform_array[7].z);

float sdf(vec3 point)
{
  float a;
  float b;
  float c;
  float r;
  vec3 h;
  vec3 i;
  vec3 j;
  vec3 k;

  if(world_toggle)
  {
    // Using mandelbox distance estimator from 2010-09-10 post by Rrrola in fractalforums, because it's better
    // than the one Warma made.
    h = point * 0.02;
    vec4 r=vec4(h,1); // distance estimate: r.w
#if defined(USE_LD)
    for(int i=0;i<int(scales[2].x);i++)
#else
      for(int i=0;i<9;i++)
#endif
      {
        r.xyz=clamp(r.xyz,-1.,1.)*2.-r.xyz; // laatikko foldaus
        c=dot(r.xyz,r.xyz);
#if defined(USE_LD)
        r*=clamp(max(scales[2].y/c,scales[2].y),.0,1.);
        r=r*vec4(vec3(scales[2].z),abs(scales[2].z))/scales[2].y+vec4(h,1.);
#else
        r*=clamp(max(.824/c,.824),.0,1.);
        r=r*vec4(vec3(-2.742),2.742)+vec4(h,1.);
#endif
      }
    r.xyz*=clamp(h.y+1,.1,1.);
#if defined(USE_LD)
    c=((length(r.xyz)-abs(scales[2].z-1.))/r.w-pow(abs(scales[2].z),float(1)-scales[2].x))*50.;
#else
    c=((length(r.xyz)-3.259)/r.w-.001475)*50.; //-pow(2.259,-8.)
#endif
  }
  else
  {
#if defined(USE_LD)
    h = noise_matrix * point * scales[0].x;
    i = noise_matrix * h * scales[0].y;
    j = noise_matrix * i * scales[0].z;
    k = noise_matrix * j * scales[0].w;
    a = texture(surface_texture, h.xz).x * scales[1].x;
    b = texture(surface_texture, i.xz).x * scales[1].y;
    c = texture(surface_texture, j.xz).x * scales[1].z;
    r = texture(surface_texture, k.xz).x * scales[1].w;
#else
    h = noise_matrix * point * 0.007;
    i = noise_matrix * h * 2.61;
    j = noise_matrix * i * 2.11;
    k = noise_matrix * j * 2.11;
    a = texture(surface_texture, h.xz).x * 2.61;
    b = texture(surface_texture, i.xz).x * 1.77;
    c = texture(surface_texture, j.xz).x * 0.11;
    r = texture(surface_texture, k.xz).x * 0.11;
#endif
    a = r + c + pow(a, 2.0) + pow(b, 2.0);
    c = length(point.xz) * 0.3;
    c = a * (smoothstep(0.0, 0.5, c * 0.0025) + 0.5) + point.y - 6.0 * ((sin(clamp(pow(c / 10.0, 1.8) - 3.14/2.0, -1.57, 1.57)) - 1.0) * 2.0 + 5.0) * cos(clamp(0.04 * c, 0.0, 3.14));
  }
  h = point - uniform_array[8].xyz;
  a = length(h);
  if(a < destruction)
  {
    return c + destruction - a;
  }
  return c;
}

float T(vec3 p, vec3 d, float I, out vec3 P, out vec3 N)
{
  vec3 n,r;
  float a = sdf(p);
  float c;
  float e;
  float i = 1.0;

  for(;i>.0;i-=I)
  {
    n = p + d * max(a * march_params.z, 0.02);
    c = sdf(n);
    if(.0>c)
    {
      for(int j=0;j<5;++j)
      {
        r=(p+n)*.5;
        e = sdf(r);
        if(.0>e)
        {
          n=r;
          c=e;
        }
        else
        {
          p=r;
          a=e;
        }
      }
      N = normalize(vec3(sdf(n.xyz + march_params.xyy).x, sdf(n.xyz + march_params.yxy).x, sdf(n.xyz + march_params.yyx).x) - c.x);
      break;
    }
    p=n;
    a=c;
  }
  P=p;
  return i;
}

float C(inout vec3 p,vec3 d,vec3 c,float r)
{
  vec3 q=p-c;
  float e=dot(q,q)-r*r,a=dot(q,d);
  if(0>e||0>a)
  {
    e=a*a-e;
    if(0<e)
    {
      p+=max(-a-sqrt(e),.0)*d;
      return length(a*d-q);
    }
  }
  return .0;
}

vec3 Q(vec3 p)
{
  vec3 a,b,c,h,i,j;
  h = noise_matrix * p;
  i = noise_matrix * h * 3.0;
  j = noise_matrix * i * 3.0;
  a = (texture(noise_volume, h).xyz - 0.5) * 2.0 * 0.6;
  b = (texture(noise_volume, i).xyz - 0.5) * 2.0 * 0.3;
  c = (texture(noise_volume, j).xyz - 0.5) * 2.0 * 0.1;
  return normalize(a+b+c);
}

void main()
{
#if defined(USE_LD)
  vec2 c=gl_FragCoord.xy*aspect.z-aspect.xy;
#elif defined(SCREEN_H) && defined(SCREEN_W) && (SCREEN_H == 1200) && (SCREEN_W == 1920)
  vec2 c=gl_FragCoord.xy/600.-vec2(1.6,1.);
#elif defined(SCREEN_H) && defined(SCREEN_W) && (SCREEN_H == 1080) && (SCREEN_W == 1920)
  vec2 c=gl_FragCoord.xy/540.-vec2(1.78,1.);
#elif defined(SCREEN_H) && defined(SCREEN_W) && (SCREEN_H == 800) && (SCREEN_W == 1280)
  vec2 c=gl_FragCoord.xy/400.-vec2(1.6,1.);
#else // Assuming 720p.
  vec2 c=gl_FragCoord.xy/360.-vec2(1.78,1.);
#endif
  vec3 p = mix(mix(uniform_array[0], uniform_array[1], uniform_array[7].y), mix(uniform_array[1], uniform_array[2], uniform_array[7].y), uniform_array[7].y) * 3.0;
  vec3 d = normalize(mix(uniform_array[3], uniform_array[4], uniform_array[7].y));
  vec3 q = mix(uniform_array[5], uniform_array[6], uniform_array[7].y);
  vec3 r = normalize(cross(d,q)),N,P;
  q=normalize(cross(r,d));
  d=normalize(d+c.x*r+c.y*q);
  q=vec3(0);
  float e;
  float n;

  r=vec3(109.,14.,86.);
  if(0 < int(uniform_array[7].z) % 2 && 0.0 < C(p, d, r, 9.0))
  {
    d=normalize(d+reflect(-d,normalize(p-r))*.2);
    destruction = -0.2;
    world_toggle = !world_toggle;
  }

  // World chosen affects iteration parameters.
  if(world_toggle)
  {
    march_params = vec4(0.05, 0.0, 0.98, 0.022);
  }

  n = T(p, d, march_params.w, P, N);
  if(.0<n)
  {
    if(world_toggle)
    {
      q = max(dot(light_direction, N), 0.0) * mix(vec3(0.3, 0.6, 0.9), vec3(1), smoothstep(-24.0, 9.0, P.y)) + pow(max(dot(d, reflect(light_direction, N)), 0.0), 7.0) * 0.11;
    }
    else
    {
      e =T(P + light_direction * 0.5, light_direction, march_params.w * 3.0, q, q);
      q = (1.0 - e) * (max(dot(light_direction, N), 0.0) * mix(vec3(0.8, 0.6, 0.4), vec3(1), smoothstep(-24.0, 9.0, P.y)) + pow(max(dot(d, reflect(light_direction, N)), 0.0), 7.0) * 0.11);
    }
    r = P - uniform_array[8];
    e = destruction + 0.5 - length(r);
    if(0<e)q+=vec3((dot(Q(P*.009),normalize(r))*.1)+.1,-.05,-.05)*smoothstep(.0,.5,e);
  }

  vec3 s=mix(vec3(.9,.8,.8),vec3(.8,.8,.9),d.y*111.*.02)*(dot(Q(p*.006+d*.1),d)*smoothstep(-.2,.5,-d.y)*.2+.8);

  if(world_toggle)
  {
    n = smoothstep(0.0, 0.4, n);
  }

  output_color = vec4(mix(mix(s, vec3(1), pow(max(dot(d, light_direction), 0.0), 7.0)), q, n), 1.0) - (int(gl_FragCoord.y * 0.5) % 2 + 0.1) * (max(max(smoothstep(0.98, 1.0, uniform_array[7].y), smoothstep(-0.02 * uniform_array[9].x, 0.0, -uniform_array[7].y) * uniform_array[9].x), 0.1) + destruction * 0.02) * dot(c, c);

  r = p;
  e = C(r, d, uniform_array[8], destruction + 0.2);

  if(0.0 < e)
  {
    output_color.xyz -= clamp(1.0 - (dot(r - p, r - p) - dot(P - p, P - p)) * 0.003, 0.0, 1.0) * (1.0 - pow(e / destruction, 5)) * (dot(Q((r - uniform_array[8]) * 0.009), d) * 0.1 + 0.9);
  }
}
