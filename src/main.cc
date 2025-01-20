#include "pch.hh"
#include "const.hh"
#include "util.hh"
#include "pid.hh"
#include "colo.hh"
#include "expression.hh"
#include "prop.hh"
#include "render.hh"
#include "plot.hh"
#include "observer.hh"
#include "rotational.hh"
#include "throttle.hh"
#include "sparkplug.hh"
#include "starter.hh"
#include "port.hh"
#include "filter.hh"
#include "gas.hh"
#include "flame.hh"
#include "audio.hh"
#include "volume.hh"
#include "node.hh"
#include "sdl.hh"
#include "ensim.hh"

int main()
{
    ensim_t{}.run();
}
