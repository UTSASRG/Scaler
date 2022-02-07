#ifndef SCALER_OBJECT_HPP
#define SCALER_OBJECT_HPP

class Object {
public:
    virtual ~Object() {
        //This should be superclass of all classes.
        //This is to make sure all deconstructors are executed normally.
    }
};

#endif
