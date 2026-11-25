




#include "../xbg_depth.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x) do {++checks;if(!(x)){if(++failures<20)printf("FAIL line %d: %s\n",__LINE__,#x);}} while(0)

static float as_float(uint32_t bits){float f;memcpy(&f,&bits,4);return f;}
static uint32_t as_bits(float f){uint32_t b;memcpy(&b,&f,4);return b;}

int main(void){
    const double M=16777215.0;
    float previous=-1.0f;
    for(uint32_t v=0;v<=0xffffffu;v++){
        uint32_t bits=xbg_depth_unorm24_to_float(v);float f=as_float(bits);

        CHECK(xbg_depth_float_to_unorm24(bits)==v);

        CHECK(f>previous);previous=f;


        double here=fabs((double)f*M-v);
        if(v)CHECK(here<=fabs((double)nextafterf(f,-1.0f)*M-v));
        if(v<0xffffffu)CHECK(here<=fabs((double)nextafterf(f,2.0f)*M-v));
    }

    CHECK(xbg_depth_unorm24_to_float(0)==0u);
    CHECK(xbg_depth_unorm24_to_float(0xffffffu)==as_bits(1.0f));
    CHECK(xbg_depth_unorm24_to_float(0xff000005u)==xbg_depth_unorm24_to_float(5));

    CHECK(xbg_depth_float_to_unorm24(as_bits(NAN))==0);
    CHECK(xbg_depth_float_to_unorm24(as_bits(-NAN))==0);
    CHECK(xbg_depth_float_to_unorm24(as_bits(-0.0f))==0);
    CHECK(xbg_depth_float_to_unorm24(as_bits(-1.0f))==0);
    CHECK(xbg_depth_float_to_unorm24(as_bits(-INFINITY))==0);
    CHECK(xbg_depth_float_to_unorm24(as_bits(1e-45f))==0);
    CHECK(xbg_depth_float_to_unorm24(as_bits(1.0f))==0xffffffu);
    CHECK(xbg_depth_float_to_unorm24(as_bits(1.5f))==0xffffffu);
    CHECK(xbg_depth_float_to_unorm24(as_bits(INFINITY))==0xffffffu);
    CHECK(xbg_depth_float_to_unorm24(as_bits(nextafterf(1.0f,0.0f)))==0xfffffeu);
    CHECK(xbg_depth_float_to_unorm24(as_bits(0.5f))==8388608u);


    for(uint32_t b=0;b<=as_bits(1.0f);b+=4099){
        uint32_t k=xbg_depth_float_to_unorm24(b);double x=(double)as_float(b)*M;
        CHECK(k<=0xffffffu);CHECK(fabs(x-k)<=0.5);
    }
    printf("depth24_float_test: %lu checks, %lu failures\n",checks,failures);
    return failures?1:0;
}
