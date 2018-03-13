#include "wstapp.h"
#include "wstrecord.h"
#include "wstconf.h"
#include "wstlog.h"

#include <csignal>
#include <thread>
#include <mutex>
#include <sys/vfs.h>
#include <sys/stat.h>

bool Application::_is_quit = false;

Application::Application()
{

}

Application::~Application()
{

}

void Application::main_thread()
{
    // http server
    httpserver http;
    http.http_server_init();
    http.http_server_run();
    http.http_server_stop();
    http.http_server_free();
}

void Application::sub1_thread()
{
    struct statfs diskinfo;
    // DIR *dir = opendir(loadconf::instance().recordpath().c_str());
    // if (dir == NULL)
    // {
    //     int beginCmpPath;
    //     int endCmpPath;
    //     int fullPathLen;
    //     int pathLen = loadconf::instance().recordpath().length();
    //     char currentPath[128] = {0};
    //     char fullPath[128] = {0};
    //     if (loadconf::instance().recordpath().at(0) != '/')
    //     {
    //         getcwd(currentPath, sizeof(currentPath));
    //         strcat(currentPath, "/");
    //         beginCmpPath = strlen(currentPath);
    //         strcat(currentPath, loadconf::instance().recordpath().c_str());
    //         if (loadconf::instance().recordpath().at(pathLen) != '/')
    //         {
    //             strcat(currentPath, "/");
    //         }
    //         endCmpPath = strlen(currentPath);
    //     }
    //     else
    //     {
    //         strcpy(currentPath, loadconf::instance().recordpath().c_str());
    //         if (loadconf::instance().recordpath().at(loadconf::instance().recordpath().length()) != '/')
    //         {
    //             strcat(currentPath, "/");
    //         }
    //         beginCmpPath = 1;
    //         endCmpPath = strlen(currentPath);
    //     }
    //     for (int i = beginCmpPath; i < endCmpPath; i++)
    //     {
    //         if (currentPath[i] == '/')
    //         {
    //             currentPath[i] = '\0';
    //             if (access(currentPath, NULL) != 0)
    //             {
    //                 if (mkdir(currentPath, 0755) == -1)
    //                 {

    //                 }
    //             }
    //             currentPath[i] = '/';
    //         }
    //     }
    // }
    // closedir(dir);
    while (!_is_quit)
    {
        statfs(loadconf::instance().recordpath().c_str(), &diskinfo);
        unsigned long long totalBlocks = diskinfo.f_bsize;
        unsigned long long freeDisk = diskinfo.f_bfree * totalBlocks;
        size_t mbFreedisk = freeDisk>>20;
        if (mbFreedisk < loadconf::instance().freedisk())
        {
            httpclient::instance().reportstatus(DISKFULL, "disk full");
        }
        // cout << loadconf::instance().recordpath() << "; free disk: " << mbFreedisk << endl;
        sleep(60*60);
    }
}

void Application::sub2_thread()
{
    while (!_is_quit)
    {
        sleep(loadconf::instance().keeplivetime());
        httpclient::instance().keeplive();
    }
}

void Application::fSignalHandler(int signum)
{
    _is_quit = true;
   
    // LOG(logger::LOGDEBUG, "server app signal handler.");
    // std::cout << "server app signal handler." << std::endl;
}


void Application::InitSignals()
{

}

int Application::Daemon()
{
    int fd;

    switch (fork())
    {
    case -1:
        return -1;

    case 0:
        break;
    
    default:
        exit(0);
    }

    if (setsid() == -1)
    {
        return -1;
    }

    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    {
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) == -1)
    {
        return -1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        return -1;
    }

    if (fd > STDERR_FILENO)
    {
        if (close(fd) == -1)
        {
            return -1;
        }
    }

    return 0;
}

int Application::ParseOption(int argc, char ** argv)
{
    char *p;
    
    for (int i = 1; i < argc; i++)
    {
        p = argv[i];
        if (*p++ != '-')
        {
            return -1;
        }

        while (*p)
        {
            switch (*p++)
            {
            case '?':
            case 'h':
                cout << "show help" << endl;
                break;

            case 'v':
                cout << "show version" << endl;
                break;

            case 'q':
                cout << "show quit" << endl;
                break;

            case 's':
                cout << "show signal" << endl;
                return -1;

            default:
                cout << "show other" << endl;
                return -1;
            }
        }
    }

    return 0;
}

int Application::Run()
{
    logger::instance().init();
    loadconf::instance().load_config_file();
    loadconf::instance().load_license_file();

    LOGW("server app start");

    // signal(SIGINT, fSignalHandler);
    std::thread worker(&Application::main_thread, this);
    std::thread sub1(&Application::sub1_thread, this);
    std::thread sub2(&Application::sub2_thread, this);
    worker.join();
    sub1.join();
    sub2.join();

    LOGW("server app stop");
    logger::instance().free();

    return 0;
}

int Application::Run(int argc, char ** argv)
{
    if (ParseOption(argc, argv) != 0)
    {
        return 1;
    }

}