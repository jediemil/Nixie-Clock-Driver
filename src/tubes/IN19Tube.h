//
// Created by emilr on 2023-03-24.
//

#ifndef NIXIECLOCK_IN19TUBE_H
#define NIXIECLOCK_IN19TUBE_H


class IN19Tube {
public:
    explicit IN19Tube(int* lookup);
    int* lookup;
};


#endif //NIXIECLOCK_IN19TUBE_H
