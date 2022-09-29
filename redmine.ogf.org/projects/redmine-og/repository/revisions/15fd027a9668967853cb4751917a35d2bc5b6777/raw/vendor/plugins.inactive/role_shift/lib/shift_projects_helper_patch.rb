require_dependency 'projects_helper'

module ShiftProjectsHelperPatch

    def self.included(base)
        base.extend(ClassMethods)
        base.send(:include, InstanceMethods)
        base.class_eval do
            unloadable
            alias_method_chain :project_settings_tabs, :roles
        end
    end

    module ClassMethods
    end

    module InstanceMethods

        def project_settings_tabs_with_roles
            tabs = project_settings_tabs_without_roles
            if User.current.allowed_to?(:manage_role_shifts, @project)
                tabs.insert(3, { :name    => 'roles',
                                 :action  => :manage_role_shifts,
                                 :partial => 'projects/settings/role_shifts',
                                 :label   => :label_role_shifts })
            end
            tabs
        end

    end

end
