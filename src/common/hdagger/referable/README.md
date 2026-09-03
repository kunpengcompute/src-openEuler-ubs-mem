### Examples
#### dg_ref.h
```
Ref is easy and high performance smart pointer compare to C++ shared_ptr. How use? 

# Step 1: declare a class is referable, there are two options:

Option 1:
class Obj : public Referable {
public:
private:
}

Option 2:
class Obj : public Referable {
public:
    DAGGER_DEFINE_REF_COUNT_FUNCTIONS

private:
    DAGGER_DEFINE_REF_COUNT_VARIABLE
}

# Step 2: declare smart pointer
using ObjPtr = Ref<Obj>;

# Step 3: use it
ObjPtr o = new (std::nothrow) Obj();
if (o == nullptr) {
    // log error
}

```