#include <utility>

template <class T>
inline Buffer<T>::Buffer(
		VertexArray *vao,
		GLenum target,
		GLenum usage,
		unsigned int itemSize,
		unsigned int numItems) :
	VAO(vao),
	Target(target),
	Usage(usage),
	ItemSize(itemSize),
	NumItems(numItems),
	Data(nullptr)
{
	glGenBuffers(1, &Name);
}

template <class T>
inline Buffer<T>::~Buffer()
{
	glDeleteBuffers(1, &Name);
	if (managed_data)
		delete[] Data;
}

template <class T>
inline Buffer<T>::Buffer(const Buffer<T>& other) :
	Name(0), Data(nullptr)
{
	*this = other;
}

template <class T>
inline Buffer<T>& Buffer<T>::operator=(const Buffer<T>& other)
{
	if (this != &other)
	{
		glDeleteBuffers(1, &Name);
		if (managed_data)
			delete[] Data;

		Name = other.Name;
		VAO = other.VAO;
		Target = other.Target;
		if (other.managed_data)
		{
			Data = new T[NumItems * ItemSize];
			memcpy(Data, other.Data, sizeof(T) * ItemSize * NumItems);
		}
		managed_data = other.managed_data;
	}
	return *this;
}

template <class T>
inline Buffer<T>::Buffer(Buffer<T>&& other) :
	Name(0), Data(nullptr)
{
	*this = std::move(other);
}

template <class T>
inline Buffer<T>& Buffer<T>::operator=(Buffer<T>&& other)
{
	if (this != &other)
	{
		glDeleteBuffers(1, &Name);
		if (managed_data)
			delete[] Data;

		Name = other.Name;
		VAO = other.VAO;
		Target = other.Target;
		Usage = other.Usage;
		ItemSize = other.ItemSize;
		NumItems = other.NumItems;
		Data = other.Data;
		other.Name = 0;
		other.VAO = nullptr;
		other.Target = 0;
		other.Usage = 0;
		other.ItemSize = 0;
		other.NumItems = 0;
		other.Data = nullptr;
	}
	return *this;
}

template <class T>
inline void Buffer<T>::SetData(T *data, bool copy)
{
	if (managed_data)
	{
		managed_data = false;
		delete[] Data;
		Data = nullptr;
	}
	if (copy && data != nullptr)
	{
		Data = new T[NumItems * ItemSize];
		memcpy(Data, data, sizeof(T) * ItemSize * NumItems);
		managed_data = true;
	}
	VAO->SetBufferData(
			Name,
			Target,
			sizeof(T) * ItemSize * NumItems,
			Data,
			Usage);
}

template <class T>
inline void Buffer<T>::BindBuffer()
{
	VAO->Bind([&](){
		glBindBuffer(Target, Name);
	});
}
