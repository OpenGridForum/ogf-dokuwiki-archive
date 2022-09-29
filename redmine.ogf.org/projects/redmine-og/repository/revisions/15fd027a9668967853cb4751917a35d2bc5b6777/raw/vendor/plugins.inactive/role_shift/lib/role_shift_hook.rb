class RoleShiftHook  < Redmine::Hook::ViewListener

    def view_layouts_base_html_head(context = {})
        stylesheet_link_tag('role_shift', :plugin => 'role_shift')
    end

end
