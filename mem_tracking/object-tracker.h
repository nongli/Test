#ifndef OBJECT_TRACKING_H
#define OBJECT_TRACKING_H

#include <map>
#include <string>
#include <sstream>

#define ENABLE_OBJECT_TRACKING(__class__) \
 : public ObjectTrackingBase<__class__> { \
 public: \
  static ObjectTracker::ObjectRefCount num_allocated; \
 \
 private:

#define INIT_OBJECT_TRACKING(__class__) \
  ObjectTracker::ObjectRefCount __class__::num_allocated(#__class__)

template<typename T>
class ObjectTrackingBase {
 public:
  virtual ~ObjectTrackingBase() {
    --T::num_allocated.ref_count_value;
  }

 protected:
  ObjectTrackingBase() {
    ++T::num_allocated.ref_count_value;
  }
};

class ObjectTracker {
 public:
  static std::string DumpObjects() {
    std::map<std::string, int*>::const_iterator it;
    std::stringstream ss;
    ss << "Active objects: " << std::endl;
    ObjectRefCount* node = objects_;
    while (node != NULL) {
      ss << "  " << node->ref_count_name << ": " << node->ref_count_value << std::endl;
      node = node->ref_count_next;
    }
    return ss.str();
  }
  
  struct ObjectRefCount {
    ObjectRefCount(const char* name) {
      ref_count_value = 0;
      ref_count_name = name;
      ref_count_next = ObjectTracker::objects_;
      ObjectTracker::objects_ = this;
    }
    
    int ref_count_value;
    const char* ref_count_name;
    
    // Linked list of these ObjectRefCount objects
    ObjectRefCount* ref_count_next;
  };

 private:
  static ObjectRefCount* objects_;
};

#endif

