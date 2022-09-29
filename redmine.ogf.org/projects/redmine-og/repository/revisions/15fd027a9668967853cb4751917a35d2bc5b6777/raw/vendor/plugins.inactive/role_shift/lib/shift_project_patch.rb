require_dependency 'project'

module ShiftProjectPatch

    def self.included(base)
        base.extend(ClassMethods)
        base.send(:include, InstanceMethods)
        base.class_eval do
            unloadable
            has_many :role_shifts, :dependent => :destroy
        end
    end

    module ClassMethods
    end

    module InstanceMethods
    end

end
