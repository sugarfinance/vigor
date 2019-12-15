/*
*   IK Nwoke 
*   This is a 2D Array class
*/

#pragma once

template <class Datatype>
class Array2d
{
    public:

    // This is a pointer to the array
    Datatype* m_array;

    // The current width of the array
    int m_width;

    // The current height of the array
    int m_height;

    
    // default ctor
    Array2d() : m_width(1), m_height(1)
    {
        // create the new array
        m_array = new Datatype[ m_width * m_height ];
    }


    // This creates the 2D array using a single linear array
    Array2d(int p_width, int p_height)
    {
        // create the new array
        m_array = new Datatype[ p_width * p_height ];

        // set the width and height
        m_width = p_width;
        m_height = p_height;
    }

    
    // This destructs the array
    ~Array2d()
    {
        // if the array is valid delete it
        if( m_array != 0 )
            delete[] m_array;
        m_array = 0;
    }

    // This retrieves the item at the X index
    Datatype& GetX(int p_x, int p_y)
    {
        return m_array[ p_y * m_width + p_x ];
    }

    // This retrieves the iten at the Y index
    /*Datatype& GetY(int p_x, int p_y)
    {
        return m_array[p_x * m_height + p_y ];
    }*/

    Datatype& GetY(int p_y)
    {
        return m_array[p_y]; // hardcoded, change later
    }

};
