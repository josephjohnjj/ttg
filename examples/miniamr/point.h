
template <int M=40, typename T=double>
class Point
{
    T value[M];

    public:

        Point()
        {
           for(int i = 0; i < M; i++)
                value[i] = 0.0;  
        }

        Point operator+(const Point& b) const 
        { 
            Point temp ;
            for(int i = 0; i < M; i++)
                temp.value[i] = value[i] + b.value[i]; 
            
            return temp; 
        }

        Point operator+(double val) const 
        { 
            Point temp ;
            for(int i = 0; i < M; i++)
                temp.value[i] = value[i] + val;
            
            return temp; 
        }

        Point operator/(double divider) const 
        { 
            Point temp ;
            for(int i = 0; i < M; i++)
                temp.value[i] = value[i] / divider;
            
            return temp; 
        }

        Point operator/(int divider) const 
        { 
            Point temp ;
            for(int i = 0; i < M; i++)
                temp.value[i] = value[i] / divider;
            
            return temp; 
        }

        Point operator*(double divider) const 
        { 
            Point temp ;
            for(int i = 0; i < M; i++)
                temp.value[i] = value[i] * divider;
            
            return temp; 
        }

        Point operator*(int divider) const 
        { 
            Point temp ;
            for(int i = 0; i < M; i++)
                temp.value[i] = value[i] * divider;
            
            return temp; 
        }
    
        void operator=(const Point& b) const 
        {
            for(int i = 0; i < M; i++)
                value[i] = b.value[i]; 
        }

        Point& operator=( const Point& b) 
        {
            for(int i = 0; i < M; i++)
                value[i] = b.value[i]; 

            return *this;
        }

        
        void operator=(const T& b) 
        {
            for(int i = 0; i < M; i++)
                value[i] = b; 
        }

};