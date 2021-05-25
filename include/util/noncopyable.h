#ifndef WEBSERVER_NONCAPYABLE_H_
#define WEBSERVER_NONCAPYABLE_H_

class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    noncopyable(const noncopyable &);
    const noncopyable &operator=(const noncopyable &);
};

#endif  //WEBSERVER_NONCAPYABLE_H_