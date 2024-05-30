#include <glog/logging.h>

#include <iostream>

int main(int argc, char* argv[]) {
    // Set verbose level
    FLAGS_v = 5;
    FLAGS_stderrthreshold = google::INFO;

    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);

    int num_cookies = 5;

    LOG(INFO) << "Found " << num_cookies << " cookies";
    LOG(ERROR) << "Found " << num_cookies << " cookies!!!";
    VLOG(1) << "I am printed when you run the program with --v=1 or higher";
    VLOG(2) << "I am printed when you run the program with --v=2 or higher";
}
