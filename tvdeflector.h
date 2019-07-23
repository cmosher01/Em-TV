#ifndef TVDEFLECTOR_H
#define TVDEFLECTOR_H


class TvDeflector {
public:
    TvDeflector(int period, int uptime, int offset = 0);

    bool is_flyback();

    float step();

    int get_period();

private:
    int period;
    int uptime;
    int curr_t;
    bool flyback;
};

#endif // TVDEFLECTOR_H
