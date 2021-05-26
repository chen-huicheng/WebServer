#ifndef WEBSERVER_NONCAPYABLE_H_
#define WEBSERVER_NONCAPYABLE_H_

class Noncopyable
{
protected:
    Noncopyable() {}
    ~Noncopyable() {}

private:
    Noncopyable(const Noncopyable &);
    const Noncopyable &operator=(const Noncopyable &);
};

#endif  //WEBSERVER_NONCAPYABLE_H_