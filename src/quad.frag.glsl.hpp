static const char *g_shader_fragment_quad = ""
#if defined(USE_LD)
"quad.frag.glsl"
#else
"#version 430\n"
"layout(location=0)uniform sampler2D n;"
"layout(location=1)uniform sampler3D x;"
"layout(location=2)uniform vec3[10] e;"
"mat3 a=mat3(.42f,-.7f,.58f,.53f,.71f,.46f,-.74f,.12f,.67f);"
"out vec4 u;"
"vec4 i=vec4(.3,.0,.7,.009);"
"vec3 l=normalize(vec3(2.,.5,-1.));"
"float f=e[9].g;"
"bool g=1.<abs(e[7].b);"
"float m(vec3 c)"
"{"
"float r,i,t,l;"
"vec3 o,v,m,s;"
"if(g)"
"{"
"o=c*.02;"
"vec4 e=vec4(o,1);"
"for(int r=0;"
"i<9;"
"r++)"
"{"
"e.rgb=clamp(e.rgb,-1.,1.)*2.-e.rgb;"
"t=dot(e.rgb,e.rgb);"
"e*=clamp(max(.824/t,.824),.0,1.);"
"e=e*vec4(vec3(-2.742),2.742)+vec4(o,1.);"
"}"
"e.rgb*=clamp(o.g+1,.1,1.);"
"t=((length(e.rgb)-3.259)/e.a-.001475)*50.;"
"}"
"else"
"{"
"o=a*c*.007;"
"v=a*o*2.61;"
"m=a*v*2.11;"
"s=a*m*2.11;"
"r=texture(n,o.rb).r*2.61;"
"i=texture(n,v.rb).r*1.77;"
"t=texture(n,m.rb).r*.11;"
"l=texture(n,s.rb).r*.11;"
"r=l+t+pow(r,2.)+pow(i,2.);"
"t=length(c.rb)*.3;"
"t=r*(smoothstep(.0,.5,t*.0025)+.5)+c.g-6.*((sin(clamp(pow(t/10.,1.8)-1.57,-1.57,1.57))-1.)*2.+5.)*cos(clamp(.04*t,.0,3.14));"
"}"
"o=c-e[8].rgb;"
"r=length(o);"
"if(a<l)return t+f-r;"
"return t;"
"}"
"float d(vec3 t,vec3 l,float f,vec3 g,vec3 n)"
"{"
"vec3 e,o;"
"float c=m(t),r,a,v=1.;"
"for(;"
"i>.0;"
"v-=f)"
"{"
"e=t+l*max(c*i.b,.02);"
"r=m(e);"
"if(.0>c)"
"{"
"for(int v=0;"
"j<5;"
"++v)"
"{"
"o=(t+e)*.5;"
"a=m(o);"
"if(.0>e)"
"{"
"e=o;"
"r=a;"
"}"
"else"
"{"
"t=o;"
"c=a;"
"}"
"}"
"n=normalize(vec3(m(e.rgb+i.rgg).r,m(e.rgb+i.grg).r,m(e.rgb+i.ggr).r)-r.x);"
"break;"
"}"
"t=e;"
"c=r;"
"}"
"g=t;"
"return v;"
"}"
"float h(vec3 c,vec3 o,vec3 v,float a)"
"{"
"vec3 e=c-v;"
"float r=dot(e,e)-a*a,t=dot(e,o);"
"if(0>e||0>a)"
"{"
"r=t*t-r;"
"if(0<e)"
"{"
"c+=max(-t-sqrt(r),.0)*o;"
"return length(t*o-e);"
"}"
"}"
"return .0;"
"}"
"vec3 b(vec3 m)"
"{"
"vec3 o,r,c,e,t,v;"
"e=a*m;"
"t=a*e*3.;"
"v=a*t*3.;"
"o=(texture(x,e).rgb-.5)*2.*.6;"
"r=(texture(x,t).rgb-.5)*2.*.3;"
"c=(texture(x,v).rgb-.5)*2.*.1;"
"return normalize(o+r+c);"
"}"
"void main()"
"{"
"vec2 n=gl_FragCoord.xy/360.-vec2(1.78,1.);"
"vec3 c=mix(mix(e[0],e[1],e[7].g),mix(e[1],e[2],e[7].g),e[7].g)*3.,t=normalize(mix(e[3],e[4],e[7].g)),r=mix(e[5],e[6],e[7].g),o=normalize(cross(t,r)),m,v;"
"r=normalize(cross(o,t));"
"t=normalize(t+n.r*o+n.g*r);"
"r=vec3(0);"
"float a,s;"
"o=vec3(109.,14.,86.);"
"if(0<int(e[7].b)%2&&.0<C(c,t,o,9.))"
"{"
"t=normalize(t+reflect(-t,normalize(c-o))*.2);"
"f=-.2;"
"g=!w;"
"}"
"if(g)i=vec4(.05,.0,.98,.022);"
"s=d(c,t,i.a,v,m);"
"if(.0<n)"
"{"
"if(g)r=max(dot(l,m),.0)*mix(vec3(.3,.6,.9),vec3(1),smoothstep(-24.,9.,v.g))+pow(max(dot(t,reflect(l,m)),.0),7.)*.11;"
"else"
"{"
"a=d(v+l*.5,l,i.a*3.,r,r);"
"r=(1.-a)*(max(dot(l,m),.0)*mix(vec3(.8,.6,.4),vec3(1),smoothstep(-24.,9.,v.g))+pow(max(dot(t,reflect(l,m)),.0),7.)*.11);"
"}"
"o=v-e[8];"
"a=f+.5-length(o);"
"if(0<e)r+=vec3((dot(b(v*.009),normalize(o))*.1)+.1,-.05,-.05)*smoothstep(.0,.5,a);"
"}"
"vec3 i=mix(vec3(.9,.8,.8),vec3(.8,.8,.9),t.g*111.*.02)*(dot(b(c*.006+t*.1),t)*smoothstep(-.2,.5,-t.g)*.2+.8);"
"if(g)s=smoothstep(.0,.4,s);"
"u=vec4(mix(mix(i,vec3(1),pow(max(dot(t,l),.0),7.)),r,s),1.)-(int(gl_FragCoord.y*.5)%2+.1)*(max(max(smoothstep(.98,1.,e[7].g),smoothstep(-.02*e[9].r,.0,-e[7].g)*e[9].r),.1)+f*.02)*dot(n,n);"
"o=c;"
"a=h(o,t,e[8],f+.2);"
"if(.0<e)u.rgb-=clamp(1.-(dot(o-c,o-c)-dot(v-c,v-c))*.003,.0,1.)*(1.-pow(a/f,5))*(dot(b((o-e[8])*.009),t)*.1+.9);"
"}"
#endif
"";
