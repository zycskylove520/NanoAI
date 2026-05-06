#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "nanoai_ncnn/cv/ncnn_cv_types.hpp"
#include "nanoai_ncnn/projects/car_project/car_pipeline.hpp"

namespace
{
    struct CliArgs
    {
        std::string param_path;
        std::string bin_path;
        std::string image_path;
        std::string output_path{"output.jpg"};
        std::string input_node{"images"};
        std::string output_node{"output0"};
    };

    void print_usage(const char *exe)
    {
        std::cout << "Usage: " << exe
                  << " --param model.param --bin model.bin --image input.jpg"
                  << " [--output output.jpg] [--input-node images] [--output-node output0]"
                  << std::endl;
    }

    bool parse_args(int argc, char **argv, CliArgs &args)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string key = argv[i];
            if (i + 1 >= argc)
            {
                return false;
            }

            if (key == "--param")
            {
                args.param_path = argv[++i];
            }
            else if (key == "--bin")
            {
                args.bin_path = argv[++i];
            }
            else if (key == "--image")
            {
                args.image_path = argv[++i];
            }
            else if (key == "--output")
            {
                args.output_path = argv[++i];
            }
            else if (key == "--input-node")
            {
                args.input_node = argv[++i];
            }
            else if (key == "--output-node")
            {
                args.output_node = argv[++i];
            }
            else
            {
                return false;
            }
        }

        return !args.param_path.empty() && !args.bin_path.empty() && !args.image_path.empty();
    }
} // namespace

int main(int argc, char **argv)
{
    CliArgs args;
    if (!parse_args(argc, argv, args))
    {
        print_usage(argv[0]);
        return 1;
    }

    cv::Mat input_image = cv::imread(args.image_path);
    if (input_image.empty())
    {
        std::cerr << "Failed to read input image: " << args.image_path << std::endl;
        return 1;
    }

    try
    {
        auto pipeline = NanoAI_NCNN::Projects::CarProject::make_car_pipeline(
            "car_project_pipeline",
            args.param_path,
            args.bin_path,
            args.input_node,
            args.output_node,
            2,
            false);

        auto detections = pipeline.run(NanoAI_NCNN::CV::NcnnPipelineInput{input_image});

        for (const auto &bbox : detections)
        {
            std::cout << "Detected: " << bbox.class_name
                      << " score=" << bbox.score
                      << " box=(" << bbox.lx << ", " << bbox.ly << ", " << bbox.rx << ", " << bbox.ry << ")"
                      << std::endl;

            cv::rectangle(input_image,
                          cv::Point(static_cast<int>(bbox.lx), static_cast<int>(bbox.ly)),
                          cv::Point(static_cast<int>(bbox.rx), static_cast<int>(bbox.ry)),
                          cv::Scalar(0, 255, 0),
                          2);

            cv::putText(input_image,
                        bbox.class_name,
                        cv::Point(static_cast<int>(bbox.lx), static_cast<int>(bbox.ly) - 10),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 255, 0),
                        1);
        }

        if (!cv::imwrite(args.output_path, input_image))
        {
            std::cerr << "Failed to write output image: " << args.output_path << std::endl;
            return 1;
        }

        std::cout << "Result image saved to: " << args.output_path << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Pipeline failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
