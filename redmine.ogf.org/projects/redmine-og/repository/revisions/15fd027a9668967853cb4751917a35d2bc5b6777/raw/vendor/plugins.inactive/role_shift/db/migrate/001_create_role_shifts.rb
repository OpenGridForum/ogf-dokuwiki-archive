class CreateRoleShifts < ActiveRecord::Migration

    def self.up
        create_table :role_shifts do |t|
            t.column :project_id, :integer, :null => false
            t.column :role_id,    :integer, :null => false
            t.column :shift_id,   :integer, :null => false
        end
        add_index :role_shifts, :project_id, :name => :role_shifts_project_ids
    end

    def self.down
        drop_table :role_shifts
    end

end
