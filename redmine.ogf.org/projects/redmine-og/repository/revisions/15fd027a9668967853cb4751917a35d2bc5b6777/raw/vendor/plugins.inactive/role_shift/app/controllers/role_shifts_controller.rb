class RoleShiftsController < ApplicationController
    model_object RoleShift
    before_filter :find_project, :only => :new
    before_filter :find_model_object, :except => :new
    before_filter :find_project_from_association, :except => :new
    before_filter :authorize

    def new
        if request.post? && params[:role_shift]
            @role_shift = RoleShift.new(params[:role_shift].merge(:project => @project))
            @role_shift.save
        end
        respond_to do |format|
            format.html { redirect_to(:controller => 'projects', :action => 'settings', :tab => 'roles', :id => @project) }
            format.js do
                render(:update) do |page|
                    page.replace_html('tab-content-roles', :partial => 'projects/settings/role_shifts')
                    page << 'hideOnLoad()'
                    page.visual_effect(:highlight, "role-shift-#{@role_shift.id}")
                end
            end
        end
    end

    def edit
        if request.post? && params[:role_shift]
            @role_shift.update_attributes(params[:role_shift])
        end
        respond_to do |format|
            format.html { redirect_to(:controller => 'projects', :action => 'settings', :tab => 'roles', :id => @project) }
            format.js do
                render(:update) do |page|
                    page.replace_html('tab-content-roles', :partial => 'projects/settings/role_shifts')
                    page << 'hideOnLoad()'
                    page.visual_effect(:highlight, "role-shift-#{@role_shift.id}")
                end
            end
        end
    end

    def destroy
        if request.post?
            @role_shift.destroy
        end
        respond_to do |format|
            format.html { redirect_to(:controller => 'projects', :action => 'settings', :tab => 'roles', :id => @project) }
            format.js do
                render(:update) do |page|
                    page.replace_html('tab-content-roles', :partial => 'projects/settings/role_shifts')
                    page << 'hideOnLoad()'
                end
            end
        end
    end

end
