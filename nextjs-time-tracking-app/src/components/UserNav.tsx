"use client";

import * as React from "react";
import { useTheme } from "next-themes";
import { Sun, Moon, Laptop, LogOut } from "lucide-react";
import { logout } from "@/app/(auth)/actions"; // Import server action
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuLabel,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
  DropdownMenuRadioGroup,
  DropdownMenuRadioItem,
} from "@/components/ui/dropdown-menu";
import { Button } from "@/components/ui/button";

interface UserNavProps {
  userEmail: string | undefined;
}

export function UserNav({ userEmail }: UserNavProps) {
  const { theme, setTheme } = useTheme();

  const handleLogout = async () => {
    await logout();
  };

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button
          variant="ghost"
          className="flex items-center w-full p-2 hover:bg-accent hover:text-accent-foreground rounded-md text-left focus:outline-none focus:ring-2 focus:ring-ring justify-start"
        >
          <div className="w-8 h-8 rounded-full bg-secondary flex items-center justify-center mr-3">
            <span className="text-sm font-medium text-secondary-foreground">
              {userEmail?.charAt(0).toUpperCase() || "U"}
            </span>
          </div>
          <div className="flex-1 min-w-0">
            <p className="text-sm font-medium text-foreground truncate">
              {userEmail || "User"}
            </p>
          </div>
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent className="w-56" align="end" forceMount>
        <DropdownMenuLabel className="font-normal">
          <div className="flex flex-col space-y-1">
            <p className="text-sm font-medium leading-none text-popover-foreground">Account</p>
            <p className="text-xs leading-none text-muted-foreground">
              {userEmail}
            </p>
          </div>
        </DropdownMenuLabel>
        <DropdownMenuSeparator />
        <DropdownMenuLabel className="text-popover-foreground">Theme</DropdownMenuLabel>
        <DropdownMenuRadioGroup value={theme} onValueChange={setTheme}>
          <DropdownMenuRadioItem value="light" className="focus:bg-accent focus:text-accent-foreground">
            <Sun className="mr-2 h-4 w-4" />
            <span>Light</span>
          </DropdownMenuRadioItem>
          <DropdownMenuRadioItem value="dark" className="focus:bg-accent focus:text-accent-foreground">
            <Moon className="mr-2 h-4 w-4" />
            <span>Dark</span>
          </DropdownMenuRadioItem>
          <DropdownMenuRadioItem value="system" className="focus:bg-accent focus:text-accent-foreground">
            <Laptop className="mr-2 h-4 w-4" />
            <span>System</span>
          </DropdownMenuRadioItem>
        </DropdownMenuRadioGroup>
        <DropdownMenuSeparator />
        <DropdownMenuItem 
          onSelect={(e) => { 
            e.preventDefault();
            handleLogout(); 
          }} 
          className="focus:bg-accent focus:text-accent-foreground cursor-pointer"
        >
          <LogOut className="mr-2 h-4 w-4" />
          <span>Log out</span>
        </DropdownMenuItem>
      </DropdownMenuContent>
    </DropdownMenu>
  );
} 