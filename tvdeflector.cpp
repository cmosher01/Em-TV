#include "tvdeflector.h"

TvDeflector::TvDeflector(int period, int uptime, int offset):
    period(period),
    uptime(uptime),
    curr_t(offset) {
}

bool TvDeflector::is_flyback() {
    return this->flyback;
}

int TvDeflector::get_period() {
    return this->period;
}

void TvDeflector::set(int period, int uptime, int offset) {
    this->period = period;
    this->uptime = uptime;
    this->curr_t = offset;
}

float TvDeflector::step() {
    const float t = this->curr_t++;
    this->curr_t %= this->period;

    const float u = this->uptime;

    this->flyback = (u < t);

    const float p = this->flyback ? this->period : 0.0f;
    return (t-p)/(u-p);
}
