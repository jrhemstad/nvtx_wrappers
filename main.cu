
#include "nvtx.hpp"


// My custom domain name
struct my_domain {
  static constexpr const char* name{"hello"};
};

using my_thread_range = nvtx::domain_thread_range<my_domain>;

int main(void){
    nvtx::domain_thread_range<my_domain> r{"msg", nvtx::argb_color{0}};

    nvtx::thread_range r2{"msg", nvtx::argb_color{0}};

    my_thread_range my_range{"msg", nvtx::argb_color{0}};
}