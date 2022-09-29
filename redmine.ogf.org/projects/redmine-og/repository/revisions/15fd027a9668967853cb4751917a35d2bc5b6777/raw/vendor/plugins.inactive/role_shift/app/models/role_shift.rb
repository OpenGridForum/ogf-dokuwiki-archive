class RoleShift < ActiveRecord::Base
    belongs_to :project
    belongs_to :role, :foreign_key => 'role_id'
    belongs_to :shift, :class_name => 'Role', :foreign_key => 'shift_id'

    validates_presence_of :project, :role, :shift
    validates_uniqueness_of :role_id, :scope => :project_id

    validate :validate_roles

    def to_role
        replacement = role.clone
        replacement.id = role.id
        replacement.name = "#{role.name} (#{shift.name})"
        if role.builtin == Role::BUILTIN_NON_MEMBER
            replacement.permissions = shift.permissions - Redmine::AccessControl.members_only_permissions
        elsif role.builtin == Role::BUILTIN_ANONYMOUS
            replacement.permissions = shift.permissions - Redmine::AccessControl.loggedin_only_permissions
        else
            replacement.permissions = shift.permissions
        end
        replacement.issues_visibility = shift.issues_visibility if replacement.has_attribute?(:issues_visibility)
        replacement
    end

private

    def validate_roles
        errors.add(:shift_id, :cant_be_the_same_as_role) if role_id == shift_id
    end

end
