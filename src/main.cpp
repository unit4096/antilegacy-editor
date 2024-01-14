#include <app.h>

int main(int argc, char **argv) {

    ale::AppConfigData configData {
        .argc = argc,
        .argv = argv
    };

    ale::App app = ale::App(configData);

    int result = app.run();

    return result;
}