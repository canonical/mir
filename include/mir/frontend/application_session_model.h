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

#ifndef MIR_FRONTEND_APPLICATION_MODEL_H_
#define MIR_FRONTEND_APPLICATION_MODEL_H_

#include "mir/frontend/application_session_container.h"

#include <memory>
#include <set>
#include <mutex>
#include <map>
#include <vector>

namespace mir
{

namespace frontend
{

class ApplicationSession;

class ApplicationSessionModel : public ApplicationSessionContainer
{
 public:
    explicit ApplicationSessionModel();
    virtual ~ApplicationSessionModel() {}
    
    void insert_session(std::shared_ptr<ApplicationSession> session);
    void remove_session(std::shared_ptr<ApplicationSession> session);
    
    void lock();
    void unlock();

    class LockingIterator : public ApplicationSessionContainer::LockingIterator
    {
    public:
      void advance();
      bool is_valid() const;
      void reset();
      const std::shared_ptr<ApplicationSession> operator*();
      virtual ~LockingIterator();
    protected:
      friend class ApplicationSessionModel;
      
      LockingIterator(
		      ApplicationSessionModel *model,
		      size_t index);

      
    private:
      ApplicationSessionModel *model;
      size_t it;
	
    };

    std::shared_ptr<ApplicationSessionContainer::LockingIterator> iterator();
    

  protected:
    ApplicationSessionModel(const ApplicationSessionModel&) = delete;
    ApplicationSessionModel& operator=(const ApplicationSessionModel&) = delete;

  private:
    std::map<std::string, std::shared_ptr<ApplicationSession>> apps;
    std::vector<std::string> apps_as_added;
    std::mutex guard;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_MODEL_H_
