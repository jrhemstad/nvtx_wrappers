
#include "nvtx.hpp"


// My custom domain name
struct my_domain {
  static constexpr const char* name{"hello"};
};

using my_thread_range = nvtx::domain_thread_range<my_domain>;

int main(void){
    nvtx::domain_thread_range<my_domain> r{"msg", nvtx::Color{nvtx::RGB{255,255,255}}};

    nvtx::thread_range r2{"msg", nvtx::Color{nvtx::ARGB{255,255,255,255}}};

    my_thread_range my_range{"msg", nvtx::Color{0}};
}