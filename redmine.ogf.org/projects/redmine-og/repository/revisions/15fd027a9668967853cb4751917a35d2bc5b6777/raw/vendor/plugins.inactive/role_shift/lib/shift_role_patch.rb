require_dependency 'role'

module ShiftRolePatch

    def self.included(base)
        base.extend(ClassMethods)
        base.send(:include, InstanceMethods)
        base.class_eval do
            unloadable
            has_many :role_shifts, :foreign_key => 'role_id',  :dependent => :destroy
            has_many :shifts,      :foreign_key => 'shift_id', :dependent => :destroy
        end
    end

    module ClassMethods
    end

    module InstanceMethods
    end

end
