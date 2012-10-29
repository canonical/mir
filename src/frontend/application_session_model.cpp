/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/frontend/application_session_model.h"
#include "mir/frontend/application_session.h"

#include <memory>
#include <cassert>
#include <algorithm>


namespace mf = mir::frontend;

mf::ApplicationSessionModel::ApplicationSessionModel()
{

}

void mf::ApplicationSessionModel::insert_session(std::shared_ptr<mf::ApplicationSession> session)
{
  std::unique_lock<std::mutex> lk(guard);
  auto name = session->get_name();

  apps[name] = session;
  apps_as_added.push_back(name);
}

void mf::ApplicationSessionModel::remove_session(std::shared_ptr<mf::ApplicationSession> session)
{
  std::unique_lock<std::mutex> lk(guard);

  apps.erase(session->get_name());
}

void mf::ApplicationSessionModel::lock()
{
  guard.lock();
}

void mf::ApplicationSessionModel::unlock()
{
  guard.unlock();
}

mf::ApplicationSessionModel::LockingIterator::LockingIterator(ApplicationSessionModel *model,
							      size_t index) : model(model),
									      it(index)
{
  
}

void mf::ApplicationSessionModel::LockingIterator::advance()
{
  it += 1;
}

void mf::ApplicationSessionModel::LockingIterator::reset()
{
  it = 0;
}

bool mf::ApplicationSessionModel::LockingIterator::is_valid() const
{
  return it < model->apps.size();
}

const std::shared_ptr<mf::ApplicationSession> mf::ApplicationSessionModel::LockingIterator::operator*()
{
  auto app = model->apps[model->apps_as_added[it]];
  return app;
}

mf::ApplicationSessionModel::LockingIterator::~LockingIterator()
{
  model->unlock();
}

std::shared_ptr<mf::ApplicationSessionContainer::LockingIterator> mf::ApplicationSessionModel::iterator()
{
  lock();
  
  std::shared_ptr<mf::ApplicationSessionModel::LockingIterator> it(
								   new mf::ApplicationSessionModel::LockingIterator(this, 0));
  
  return it;
}


  
