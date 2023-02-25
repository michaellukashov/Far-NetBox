//
// Created by ouyangliduo on 2017/1/7.
//

#include <tinylog/LockFreeQueue.h>
#include <tinylog/platform_win32.h>

#define interlockedcompareexchange InterlockedCompareExchange
#define interlockedexchange InterlockedExchange

//#define CAS(ptr, old_value, new_value) __sync_val_compare_and_swap(ptr, old_value, new_value)
#define CAS(ptr, old_value, new_value) interlockedcompareexchange((uint64_t volatile *)ptr, (uint64_t)new_value, (uint64_t)old_value)

LockFreeQueue::LockFreeQueue()
{
  Node* dumb = new Node();
  dumb->next_ = nullptr;
  head_ = tail_ = dumb;
}

void LockFreeQueue::Push(std::string& data)
{
  Node* new_node = new Node();
  new_node->data_ = data;
  new_node->next_ = nullptr;
  Node* tail;
  Node* next;

  while(true)
  {
    tail = tail_;
    next = tail->next_;

    if(tail != tail_)
    {
      continue;
    }

    if(next != nullptr)
    {
      CAS(&tail_, tail, next);
      continue;
    }

    if(CAS(&tail->next_, next, new_node))
    {
      break;
    }
  }

  CAS(&tail_, tail, new_node);
}

int32_t LockFreeQueue::Pop(std::string& data)
{
  Node* head;
  Node* tail;
  Node* next;

  while(true)
  {
    head = head_;
    tail = tail_;
    next = head->next_;

    if(head != head_)
    {
      continue;
    }

    if(next == nullptr)
    {
      return -1;
    }

    if(head == tail)
    {
      CAS(&tail_, tail, next);
      continue;
    }
    data = next->data_;
    if(CAS(&head_, head, next))
    {
      break;
    }
  }

  delete head;
  return 0;
}
