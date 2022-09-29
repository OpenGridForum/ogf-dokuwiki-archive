require_dependency 'user'

module ShiftUserPatch

    def self.included(base)
        base.extend(ClassMethods)
        base.send(:include, InstanceMethods)
        base.class_eval do
            unloadable
            alias_method_chain :roles_for_project, :shift
        end
    end

    module ClassMethods
    end

    module InstanceMethods

        def roles_for_project_with_shift(project)
            roles = roles_for_project_without_shift(project)

            if project.module_enabled?(:role_shifts)
                shifts = RoleShift.find(:all, :conditions => { :project_id => project.id }).inject({}) do |hash, shift|
                    hash[shift.role.id] = shift
                    hash
                end

                roles.collect do |role|
                    shifts[role.id] ? shifts[role.id].to_role : role
                end
            else
                roles
            end
        end

    end

end
