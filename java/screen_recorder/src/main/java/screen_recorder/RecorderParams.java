package screen_recorder;

import com.google.gson.Gson;

public class RecorderParams
{
    public int desktop_num;
    public int pid;

    public RecorderParams ()
    {
        desktop_num = 0;
        pid = 0;
    }

    public String to_json ()
    {
        return new Gson ().toJson (this);
    }

    public int get_desktop_num ()
    {
        return desktop_num;
    }

    public void set_desktop_num (int desktop_num)
    {
        this.desktop_num = desktop_num;
    }

    public int get_pid ()
    {
        return pid;
    }

    public void set_pid (int pid)
    {
        this.pid = pid;
    }
}
