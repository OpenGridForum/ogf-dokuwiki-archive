require 'redmine'
require 'dispatcher'

require_dependency 'role_shift_hook'

RAILS_DEFAULT_LOGGER.info 'Starting Role Shift Plugin for Redmine'

Dispatcher.to_prepare :role_shift_plugin do
    unless User.included_modules.include?(ShiftUserPatch)
        User.send(:include, ShiftUserPatch)
    end
    unless Project.included_modules.include?(ShiftProjectPatch)
        Project.send(:include, ShiftProjectPatch)
    end
    unless Role.included_modules.include?(ShiftRolePatch)
        Role.send(:include, ShiftRolePatch)
    end
    unless ProjectsHelper.included_modules.include?(ShiftProjectsHelperPatch)
        ProjectsHelper.send(:include, ShiftProjectsHelperPatch)
    end
end

Redmine::Plugin.register :role_shift_plugin do
    name 'Role Shift'
    author 'Andriy Lesyuk'
    author_url 'http://www.andriylesyuk.com/'
    description 'Lets managing roles (including "Non members" and "Anonymous") at project level.'
    url 'http://projects.andriylesyuk.com/projects/role-shift'
    version '0.0.1b'

    project_module :role_shifts do
        permission :manage_role_shifts, { :role_shifts => [ :new, :edit, :destroy ] }, :require => :member
    end
end
