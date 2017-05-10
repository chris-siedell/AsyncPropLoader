# AsyncPropLoader
A loader for the Parallax Propeller P8X32A microcontroller.

This project is in active development and I expect API breaking changes May-August 2017.

I intend to improve the error reporting mechanism to provide more informative, actionable error messages.

Full documentation (including internal/private stuff):  
http://siedell.com/projects/AsyncPropLoader/docs/full/

Dependencies:  
https://github.com/wjwwood/serial  
https://github.com/chris-siedell/HSerial

Example usage:
```cpp

#include <iostream>

#include "AsyncPropLoader.hpp"
#include "SimpleErrors.hpp"

using std::cout;

using APLoader::AsyncPropLoader;

class LoaderDemo : public APLoader::StatusMonitor {
public:

    void startLoadDemo(const char* portDeviceName, uint32_t baudrate, const std::vector<uint8_t>& image) {

        AsyncPropLoader loader(portDeviceName);
        loader.setBaudrate(baudrate);
        loader.setStatusMonitor(this);

        cout << "Will attempt to load image with " << image.size() << " bytes\n";

        try {
            loader.loadRAM(image);
        } catch (const std::invalid_argument& e) {
            cout << "Error: The image is invalid. " << e.what() << "\n";
            return;
        } catch (const simple::IsBusyError& e) {
            cout << "Error: The loader is busy. " << e.what() << "\n";
            return;
        }

        // At this point the loader is performing the action on another thread.
        // Updates are posted using the loader* callbacks defined by StatusMonitor.

        loader.waitUntilFinished(); // Default value 0 disables timeout.
    }

    // In real applications the callbacks should probably redispatch their work to other threads.
    // This is because callbacks are made on the same thread that is using the serial port, and
    // updates are provided at the transitions between stages, not at regular intervals.

    void loaderWillBegin(AsyncPropLoader& loader, APLoader::Action action,
                         float timeTaken, float estimatedTime) noexcept override {
        cout << "Loader will begin, action: " << APLoader::strForAction(action) << "\n";
        cout << " time so far: " << timeTaken << " s, estimated time: "
            << estimatedTime << ", done: " << (100.0*timeTaken/estimatedTime) << "%\n";
    }

    void loaderUpdate(AsyncPropLoader& loader, APLoader::Status status,
                      float timeTaken, float estimatedTime) noexcept override {
        cout << "Loader update, status: " << APLoader::strForStatus(status) << "\n";
        cout << " time so far: " << timeTaken << " s, estimated time: "
            << estimatedTime << ", done: " << (100.0*timeTaken/estimatedTime) << "%\n";
    }

    void loaderHasFinished(AsyncPropLoader& loader,
                           APLoader::ErrorCode errorCode, const std::string& details,
                           const APLoader::ActionSummary& summary) noexcept override {
        // (The arguments of this call may change after further development.)
        if (errorCode == APLoader::ErrorCode::None) {
            cout << "Loader finished OK, time taken: " << summary.totalTime << " s\n";
        } else {
            cout << "Loader finished with error, time taken: " << summary.totalTime << " s\n";
            cout << " error: " << APLoader::strForErrorCode(errorCode) << "\n";
            cout << " details: " << details << "\n";
        }
    }
};
```
