#include "benchmark.hpp"
#include <libvisual/libvisual.h>
#include <libvisual/lv_util.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace {

  class MorphBench
      : public LV::Tools::Benchmark
  {
  public:

      MorphBench (std::string const& morph_name, unsigned int width, unsigned int height, VisVideoDepth depth)
          : Benchmark { "MorphBench" }
          , m_morph { visual_morph_new (morph_name.c_str ()) }
      {
          if (!m_morph) {
              throw std::invalid_argument ("Cannot load morph " + morph_name);
          }

          visual_morph_realize (m_morph);

          m_dest = LV::Video::create (width, height, depth);
          m_src1 = LV::Video::create (width, height, depth);
          m_src2 = LV::Video::create (width, height, depth);

          visual_morph_set_video (m_morph, m_dest.get ());
      }

      virtual void operator() (unsigned int max_runs)
      {
          float rate = 0.0;

          for (unsigned int i = 0; i < max_runs; i++) {
              visual_morph_set_rate (m_morph, rate);
              visual_morph_run (m_morph, &m_audio, m_src1.get (), m_src2.get ());

              rate += 0.1;

              if (rate > 1.0)
                  rate = 0.0;
          }
      }

      virtual ~MorphBench ()
      {
          visual_object_unref (VISUAL_OBJECT (m_morph));
      }

  private:

      VisMorph*    m_morph;
      LV::Audio    m_audio;
      LV::VideoPtr m_dest;
      LV::VideoPtr m_src1;
      LV::VideoPtr m_src2;
  };

  std::unique_ptr<MorphBench> make_benchmark (int& argc, char** argv)
  {
      std::string   morph_name = "slide_up";
      unsigned int  width      = 640;
      unsigned int  height     = 480;
      VisVideoDepth depth      = VISUAL_VIDEO_DEPTH_32BIT;

      if (argc > 1) {
          morph_name = argv[1];
          argc--; argv++;
      }

      if (argc > 2) {
          int value1 = std::atoi (argv[1]);
          int value2 = std::atoi (argv[2]);

          if (value1 <= 0 || value2 <= 0) {
              throw std::invalid_argument ("Invalid dimensions specified");
          }

          width  = value1;
          height = value2;

          argc -= 2; argv += 2;
      }

      if (argc > 1) {
          depth = visual_video_depth_enum_from_value (std::atoi (argv[1]));
          if (depth == VISUAL_VIDEO_DEPTH_NONE) {
              throw std::invalid_argument ("Invalid video depth specified");
          }

          argc--; argv++;
      }

      return LV::make_unique<MorphBench> (morph_name, width, height, depth);
  }

} // anonymous

int main (int argc, char **argv)
{
    try {
        LV::System::init (argc, argv);

        unsigned int max_runs = 1000;

        if (argc > 1) {
            int value = std::atoi (argv[1]);
            if (value <= 0) {
                throw std::invalid_argument ("Number of runs is non-positive");
            }

            max_runs = value;

            argc--; argv++;
        }

        auto benchmark = make_benchmark (argc, argv);
        LV::Tools::run_benchmark (*benchmark, max_runs);

        return EXIT_SUCCESS;
    }
    catch (std::exception& error) {
        std::cerr << "Exception caught: " << error.what () << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown exception caught\n";
        return EXIT_FAILURE;
    }
}
