template<class Type>
class Buffer
{
public:
    Buffer(int size = 1)
    {
        if (size < 1)
        {
            currentSize = 0;
            return;
        }

        _array = new (std::nothrow) Type[size];
        if (_array == nullptr)
            currentSize = 0;
        else
            currentSize = size;
    }

    bool resize(int size)
    {
        if (_array != nullptr)
            delete[] _array;
        currentSize = 0;

        if (size < 1)
            return false;

        _array = new (std::nothrow) Type[size];
        
        if (_array == nullptr)
            return false;

        currentSize = size;
        return true;
    }

    Type* data()
    {
        return _array;
    }

    Type& operator[](int idx)
    {
        return *(_array + idx);
    }

    bool valid() { return _array != nullptr; }

    ~Buffer()
    {
        if (_array != nullptr)
            delete[] _array;
    }
private:
    Type* _array { nullptr };
    int currentSize;
};