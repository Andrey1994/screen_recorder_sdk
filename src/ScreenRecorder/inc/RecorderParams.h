#pragma once


struct RecorderParams
{
    int pid;
    int desktopNum;

    RecorderParams (int pid, int desktopNum)
    {
        this->pid = pid;
        this->desktopNum = desktopNum;
    }

    RecorderParams ()
    {
        this->pid = 0;
        this->desktopNum = 0;
    }
};