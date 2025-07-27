#pragma once

#include "../include/State.h"

class ProvisionState : public State
{
public:
  ProvisionState();
  virtual ~ProvisionState();
  
  const char* getStateName() const override { return "ProvisionState"; }

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "ProvisionState"; }

private:
  bool apModeStarted;
};