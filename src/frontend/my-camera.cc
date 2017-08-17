#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>

#include <mutex>
#include <list>
#include <thread>
#include <memory>

#include "raster.hh"
#include "display.hh"
#include "camera.hh"

using namespace std;

int main( int argc, char const * argv[] )
{
    const uint16_t width = 1280;
    const uint16_t height = 720;

    Camera camera( width, height, 1<<20, 24, V4L2_PIX_FMT_MJPEG, "/dev/video1" );

    std::mutex q_lock;
    std::list<std::shared_ptr<BaseRaster>> q;
    
    std::thread t([&](){
            BaseRaster raster( width, height, width, height );
            VideoDisplay display( raster );
            std::shared_ptr<BaseRaster> r;

            while(true){
                bool set = false;
                {
                    std::lock_guard<std::mutex> lg(q_lock);
                    if(q.size() > 0){
                        r = q.front();
                        q.pop_front();
                        set = true;
                    }
                }
                if(set){
                    auto display_raster_t1 = std::chrono::high_resolution_clock::now();
                    display.draw( *r.get() );
                    auto display_raster_t2 = std::chrono::high_resolution_clock::now();
                    auto display_raster_time = std::chrono::duration_cast<std::chrono::duration<double>>(display_raster_t2 - display_raster_t1);
                    std::cout << "display_raster: " << display_raster_time.count() << "\n";
                }
            }
        });
        
    while ( true ) {
        auto get_raster_t1 = std::chrono::high_resolution_clock::now();

        std::shared_ptr<BaseRaster> r = std::unique_ptr<BaseRaster>(new BaseRaster{width, height, width, height});
        camera.get_next_frame( *r.get() );
        {
            std::lock_guard<std::mutex> lg(q_lock);
            if(q.size() < 1){
                q.push_back(r);
            }
        }
        auto get_raster_t2 = std::chrono::high_resolution_clock::now();
        auto get_raster_time = std::chrono::duration_cast<std::chrono::duration<double>>(get_raster_t2 - get_raster_t1);
        //std::cout << "get_raster: " << get_raster_time.count() << "\n";
        usleep(1000);
    }        

    return 0;
}
