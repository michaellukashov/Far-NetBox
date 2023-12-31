#pragma once

#include <string>
#include <nbglobals.h>

class Node
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  std::string data_;
  Node* next_;
};

class LockFreeQueue
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  LockFreeQueue();

  ~LockFreeQueue() = default;

  void Push(std::string & data);

  int32_t Pop(std::string & data);

private:
  Node * head_{nullptr};
  Node * tail_{nullptr};
};
