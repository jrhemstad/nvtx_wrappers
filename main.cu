
#include "nvtx3.hpp"


// My custom domain name
struct my_domain {
  static constexpr const char* name{"hello"};
};

struct my_message{
    static constexpr const char* message{"my message"};
};

using my_thread_range = nvtx3::domain_thread_range<my_domain>;

int main(void){
    NVTX_FUNC_RANGE();

    /*
    nvtx3::domain_thread_range<my_domain> r{"msg", nvtx3::Color{nvtx3::RGB{255,255,255}}};

    nvtx3::thread_range r2{"msg", nvtx3::Color{nvtx3::ARGB{255,255,255,255}}};

    my_thread_range my_range{"msg", nvtx3::Color{0}};

    auto registered_msg = nvtx3::get_registered_message<my_message, my_domain>();
    */

    nvtx3::EventAttributes attr{"msg", nvtx3::Color{0}};

    nvtx3::domain_thread_range<my_domain> r2{attr};

    nvtx3::domain_thread_range<my_domain> r3{"msg", nvtx3::Color{0}};

    nvtx3::thread_range r4{attr};

}