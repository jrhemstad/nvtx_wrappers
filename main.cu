
#include "nvtx.hpp"


// My custom domain name
struct my_domain {
  static constexpr const char* name{"hello"};
};

struct my_message{
    static constexpr const char* message{"my message"};
};

using my_thread_range = nvtx::domain_thread_range<my_domain>;

int main(void){

    /*
    nvtx::domain_thread_range<my_domain> r{"msg", nvtx::Color{nvtx::RGB{255,255,255}}};

    nvtx::thread_range r2{"msg", nvtx::Color{nvtx::ARGB{255,255,255,255}}};

    my_thread_range my_range{"msg", nvtx::Color{0}};

    auto registered_msg = nvtx::get_registered_message<my_message, my_domain>();
    */

    nvtx::EventAttributes attr{"msg", nvtx::Color{0}};

    nvtx::domain_thread_range<my_domain> r2{attr};

    nvtx::domain_thread_range<my_domain>{"msg", nvtx::Color{0}};


    NVTX_TRACE_FUNCTION();

}